//
//  main.mm
//

#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <CoreGraphics/CGContext.h>


// COGS_OBJECTIVE_C_CODE should be defined by only 1 source file, to generate Obj-C code that needs to be linkable
#define COGS_OBJECTIVE_C_CODE
#import "cogs/main.hpp"
#import "cogs/os/gui/nsview.hpp"


@interface AppDelegate : NSObject <NSApplicationDelegate>
@end


@interface AppDelegate ()
@end


@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	// Insert code here to initialize your application

	cogs::initialize();

	(void)cogs::alt_main();

	auto dq = cogs::quit_dispatcher::get();
	dq->get_condition().dispatch([]()
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


int main(int argc, const char * argv[]) {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    __strong NSString* appName = [[NSProcessInfo processInfo] processName];
    __strong NSString* quitTitle = [@"Quit " stringByAppendingString:appName];
    __strong NSMenuItem* quitMenuItem = [[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"];
    __strong NSMenu* appMenu = [NSMenu new];
    [appMenu addItem:quitMenuItem];
    __strong NSMenuItem* appMenuItem = [NSMenuItem new];
    [appMenuItem setSubmenu:appMenu];
    __strong NSMenu* menuBar = [NSMenu new];
    [menuBar addItem:appMenuItem];
    [NSApp setMainMenu:menuBar];

    __strong AppDelegate* appDelegate = [[AppDelegate alloc] init];
    [NSApp setDelegate:appDelegate];
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp run];
    return EXIT_SUCCESS;
}
