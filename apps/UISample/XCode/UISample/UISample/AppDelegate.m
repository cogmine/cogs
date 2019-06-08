//
//  AppDelegate.m
//  UISample
//
//  Created by Colen Garoutte-Carson on 3/2/19.
//  Copyright © 2019 Colen Garoutte-Carson. All rights reserved.
//

#import "AppDelegate.h"
#import "cogs/main.hpp"

@interface AppDelegate ()

@property (weak) IBOutlet NSWindow *window;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	// Insert code here to initialize your application

	cogs::initialize();

	(void)COGS_MAIN();

	cogs::get_quit_event()->dispatch([]()
	{
		NSApplication* app = [NSApplication sharedApplication];
		[app performSelectorOnMainThread : @selector(terminate : ) withObject:app waitUntilDone : NO] ;
	});
}


- (void)applicationWillTerminate:(NSNotification *)aNotification {
	// Insert code here to tear down your application

	atexit(cogs::terminate);
}


@end
