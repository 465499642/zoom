/*
 *  A Z-Machine
 *  Copyright (C) 2000 Andrew Hunter
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * General ZMachine utility functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "zmachine.h"
#include "file.h"
#include "zscii.h"
#include "display.h"
#include "rc.h"

void zmachine_load_story(char* filename, ZMachine* machine)
{
#ifdef PAGED_MEMORY
#else
  ZDWord size;
  ZFrame* frame;

  machine->story_length = size = get_file_size(filename);
  if (size < 0)
    zmachine_fatal("Unable to open story file");
  if (size < 64)
    zmachine_fatal("Story file is way too small (%i bytes)", size);

  machine->file = open_file(filename);
  machine->memory = read_block(machine->file, 0, size);
  /* close_file(machine->file); */
#endif

#ifdef GLOBAL_PC
  machine->pc = -1;
#endif
  
  machine->stack.stack_size    = 2048;
  machine->stack.stack_total   = 2048;
  machine->stack.stack         = malloc(sizeof(ZWord)*2048);
  machine->stack.stack_top     = machine->stack.stack;

  /*
   * Topmost frame is a 'fake' frame to make quetzal work properly
   */
  frame = machine->stack.current_frame = malloc(sizeof(ZFrame));

  frame->ret          = 0;
  frame->flags        = 0;
  frame->storevar     = 0;
  frame->discard      = 0;
  frame->frame_size   = 0;
  frame->last_frame   = NULL;
  frame->frame_num    = 0;
  frame->nlocals      = 0;
  frame->v4read       = NULL;
  frame->v5read       = NULL;
  
  machine->header = machine->memory;

  machine->dynamic_ceiling     = (ZUWord)GetWord(machine->header, ZH_static);
  machine->buffering           = 1;

  machine->globals             = machine->memory +
    GetWord(machine->header, ZH_globals);
  machine->objects             = machine->memory +
    GetWord(machine->header, ZH_objs);
  machine->dict                = machine->memory +
    GetWord(machine->header, ZH_dict);

  machine->cached_dictionaries = hash_create();

  machine->routine_offset = 8*GetWord(machine->header, ZH_routines);
  machine->string_offset = 8*GetWord(machine->header, ZH_staticstrings);

  /* Parse the abbreviations table */
  {
    ZByte* abbrev;
    char*  word;
    int x, len;

    abbrev = machine->memory + GetWord(machine->header, ZH_abbrevs);

    for (x=0; x<96*2; x+=2)
      {
	word = zscii_to_ascii(machine->memory +
			      ((abbrev[x]<<9)|(abbrev[x+1]<<1)), &len);
	machine->abbrev[x>>1] = malloc(strlen(word)+1);
	strcpy(machine->abbrev[x>>1], word);
      }
  }

  machine->screen_on = 1;
  machine->transcript_on = 0;
  machine->transcript_file = NULL;
  machine->script_on = 0;
  machine->script_file = NULL;
  machine->memory_on = 0;
}

void zmachine_fatal(char* format, ...)
{
  va_list* ap;
  char     string[256];

  va_start(ap, format);
  vsprintf(string, format, ap);
  va_end(ap);

  if (machine.display_active)
    {
      machine.display_active = 0;

      display_set_font(0);

      display_set_style(0);
      display_set_style(2);
      display_set_colour(7, 0);
      display_prints_c("\n\n");
      display_set_colour(3, 1);
      display_printf("INTERPRETER PANIC: %s", string);
#ifdef GLOBAL_PC
      display_printf(" (PC = #%x)", machine.pc);
#endif
      display_set_colour(7, 0);
      display_set_style(0);
      display_prints_c("\n\n[Press any key to exit]\n");
      display_readchar(0);
    }
  else
    {
      fprintf(stderr, "\nINTERPRETER PANIC - %s", string);
#ifdef GLOBAL_PC
      fprintf(stderr, " (PC = #%x)\n\n", machine.pc);
#endif
    }
  
  exit(1);
}

void zmachine_warning(char* format, ...)
{
  va_list* ap;
  char     string[256];

  if (machine.warning_level == 0)
    return;
  
  va_start(ap, format);
  vsprintf(string, format, ap);
  va_end(ap);

  if (machine.warning_level == 2)
    zmachine_fatal("WARNING - %s", string);

  if (machine.display_active)
    {
      display_printf("[ WARNING - %s", string);
#ifdef GLOBAL_PC
      display_printf(" (PC = #%x)", machine.pc);
#endif
      display_prints_c(" ]\n");
    }
  else
    {
      fprintf(stderr, "[ WARNING - %s", string);
#ifdef GLOBAL_PC
      fprintf(stderr, " (PC = #%x)", machine.pc);
#endif
      fprintf(stderr, " ]\n");
    }
#ifdef DEBUG
  fprintf(stderr, "\nWARNING - %s", string);
#ifdef GLOBAL_PC
  fprintf(stderr, " (PC = #%x)", machine.pc);
#endif
  fprintf(stderr, "\n\n");
#endif
}

#define Flag(p, f, v) machine.memory[p] = \
    (v)?machine.memory[p]|(1<<(f)): \
    machine.memory[p]&~(1<<(f))

void zmachine_setup_header(void)
{
  machine.dinfo = display_get_info();

  if (machine.memory[0]<5)
    machine.graphical = 0;
  
  switch (machine.memory[0])
    {
    case 6:
      Flag(1, 0, machine.dinfo->colours);
      machine.memory[ZH_width] = machine.dinfo->width>>8;
      machine.memory[ZH_width+1] = machine.dinfo->width;
      machine.memory[ZH_height] = machine.dinfo->height>>8;
      machine.memory[ZH_height+1] = machine.dinfo->height;
      /* Note that these are backwards in v6 :-) */
      machine.memory[ZH_fontwidth] = machine.dinfo->font_height;
      machine.memory[ZH_fontheight] = machine.dinfo->font_width;

      Flag(1, 2, machine.dinfo->boldface);
      Flag(1, 3, machine.dinfo->italic);
      Flag(1, 4, machine.dinfo->fixed_space);
      Flag(1, 4, machine.dinfo->timed_input);
      Flag(1, 5, 0);
      Flag(1, 5, machine.dinfo->pictures);

      machine.memory[ZH_intnumber] = 1;
      machine.memory[ZH_intvers] = 1;
      machine.memory[ZH_lines] = machine.dinfo->lines;
      machine.memory[ZH_columns] = machine.dinfo->columns;

      machine.memory[ZH_intnumber] = rc_get_interpreter();
      machine.memory[ZH_intvers] = rc_get_revision();
      break;
      
    case 8:
    case 7:
    case 5:
      Flag(1, 0, machine.dinfo->colours);
      machine.memory[ZH_deffore]    = machine.dinfo->fore+2;
      machine.memory[ZH_defback]    = machine.dinfo->back+2;

      if (!machine.graphical)
	{
	  machine.memory[ZH_width]      = machine.dinfo->columns>>8;
	  machine.memory[ZH_width+1]    = machine.dinfo->columns;
	  machine.memory[ZH_height]     = machine.dinfo->lines>>8;
	  machine.memory[ZH_height+1]   = machine.dinfo->lines;
	  machine.memory[ZH_fontwidth]  = 1;
	  machine.memory[ZH_fontheight] = 1;
	}
      else
	{
	  machine.memory[ZH_width]      = machine.dinfo->width>>8;
	  machine.memory[ZH_width+1]    = machine.dinfo->width;
	  machine.memory[ZH_height]     = machine.dinfo->height>>8;
	  machine.memory[ZH_height+1]   = machine.dinfo->height;
	  machine.memory[ZH_fontwidth]  = machine.dinfo->font_width;
	  machine.memory[ZH_fontheight] = machine.dinfo->font_height;	  
	}
    case 4:
      Flag(1, 2, machine.dinfo->boldface);
      Flag(1, 3, machine.dinfo->italic);
      Flag(1, 4, machine.dinfo->fixed_space);
      Flag(1, 7, machine.dinfo->timed_input);

      machine.memory[ZH_lines]     = machine.dinfo->lines;
      machine.memory[ZH_columns]   = machine.dinfo->columns;

      machine.memory[ZH_intnumber] = rc_get_interpreter();
      machine.memory[ZH_intvers]   = rc_get_revision();
      break;

    case 3:
      Flag(1, 4, !machine.dinfo->status_line);
      Flag(1, 5, machine.dinfo->can_split);
      Flag(1, 6, machine.dinfo->variable_font);
      break;
    }
}

#ifdef DEBUG
extern ZWord debug_print_var(ZWord val, int var)
{
  printf("Read variable #%x (value %i)\n", var, val);
  return val;
}
#endif
