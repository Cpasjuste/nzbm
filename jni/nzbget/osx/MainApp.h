/*
 *  This file is part of nzbget
 *
 *  Copyright (C) 2007-2013 Andrey Prygunkov <hugbug@users.sourceforge.net>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * $Revision: 807 $
 * $Date: 2013-08-31 23:14:39 +0200 (Sat, 31 Aug 2013) $
 *
 */

#import <Cocoa/Cocoa.h>
#import "DaemonController.h"

@interface MainApp : NSObject <NSMenuDelegate, DaemonControllerDelegate> {
	IBOutlet NSMenu *statusMenu;
    NSStatusItem *statusItem;
	IBOutlet NSMenuItem *webuiItem;
	IBOutlet NSMenuItem *homePageItem;
	IBOutlet NSMenuItem *downloadsItem;
	IBOutlet NSMenuItem *forumItem;
	IBOutlet NSMenuItem *info1Item;
	IBOutlet NSMenuItem *info2Item;
	IBOutlet NSMenuItem *restartRecoveryItem;
	IBOutlet NSMenuItem *factoryResetItem;
	IBOutlet NSMenuItem *destDirItem;
	IBOutlet NSMenuItem *interDirItem;
	IBOutlet NSMenuItem *nzbDirItem;
	IBOutlet NSMenuItem *scriptDirItem;
	IBOutlet NSMenuItem *configFileItem;
	IBOutlet NSMenuItem *logFileItem;
	IBOutlet NSMenuItem *destDirSeparator;
	NSWindowController *welcomeDialog;
	NSWindowController *preferencesDialog;
	DaemonController *daemonController;
	int connectionAttempts;
	BOOL restarting;
	BOOL resetting;
	NSTimer* restartTimer;
	NSMutableArray* categoryItems;
	NSMutableArray* categoryDirs;
}

+ (void)setupAppDefaults;

- (void)setupDefaultsObserver;

- (IBAction)quitClicked:(id)sender;

- (IBAction)preferencesClicked:(id)sender;

- (void)userDefaultsDidChange:(id)sender;

- (IBAction)aboutClicked:(id)sender;

+ (BOOL)wasLaunchedAsLoginItem;

- (IBAction)webuiClicked:(id)sender;

- (IBAction)infoLinkClicked:(id)sender;

- (IBAction)openConfigInTextEditClicked:(id)sender;

- (IBAction)restartClicked:(id)sender;

- (IBAction)showInFinderClicked:(id)sender;

@end
