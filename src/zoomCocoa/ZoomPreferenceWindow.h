//
//  ZoomPreferenceWindow.h
//  ZoomCocoa
//
//  Created by Andrew Hunter on Sat Dec 20 2003.
//  Copyright (c) 2003 Andrew Hunter. All rights reserved.
//

#import <AppKit/AppKit.h>

#import "ZoomPreferences.h"

@interface ZoomPreferenceWindow : NSWindowController {
	// The various views
	IBOutlet NSView* generalSettingsView;
	IBOutlet NSView* gameSettingsView;
	IBOutlet NSView* fontSettingsView;
	IBOutlet NSView* colourSettingsView;
	
	// The settings controls themselves
	IBOutlet NSButton* displayWarnings;
	IBOutlet NSButton* fatalWarnings;
	IBOutlet NSButton* speakGameText;
	
	IBOutlet NSTextField* gameTitle;
	IBOutlet NSPopUpButton* interpreter;
	IBOutlet NSTextField* revision;
	
	IBOutlet NSTableView* fonts;
	IBOutlet NSTableView* colours;
	
	// The toolbar
	NSToolbar* toolbar;
	
	// The preferences that we're editing
	ZoomPreferences* prefs;
}

- (void) setPreferences: (ZoomPreferences*) prefs;

// Interface actions
- (IBAction) titleChanged: (id) sender;
- (IBAction) interpreterChanged: (id) sender;
- (IBAction) revisionChanged: (id) sender;
- (IBAction) displayWarningsChanged: (id) sender;
- (IBAction) fatalWarningsChanged: (id) sender;
- (IBAction) speakGameTextChanged: (id) sender;

@end
