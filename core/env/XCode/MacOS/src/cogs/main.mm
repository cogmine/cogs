//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Obsolete


#import <Cocoa/Cocoa.h>
#include "cogs/mem/rc.hpp"
#include "cogs/threads.hpp"
#include "cogs/quit.hpp"
#include "cogs/os/gui/nsview.hpp"

using namespace cogs;


int main(int argc, char *argv[])
{
    return NSApplicationMain(argc,  (const char **) argv);
}


@interface CogsApp : NSObject
{
}

-(void)applicationDidFinishLaunching:(NSNotification *)notification;
-(void)applicationWillTerminate:(NSNotification *)notification;


@end

namespace cogs {
	int main();
}


void do_cleanup()
{
	cogs::ui::os::drain_ui_thread_deferals();
	cogs::thread_pool::shutdown_default();
	cogs::ui::os::drain_ui_thread_deferals();

	cogs::allocator::shut_down();
//	allocator::get_cleanup_event()->wait();
}


@implementation CogsApp

-(void)applicationDidFinishLaunching:(NSNotification *) notification
{
	result = cogs::main();
	cogs::wait_for_quit();
}

-(void)applicationWillTerminate:(NSNotification *)notification
{
	atexit(do_cleanup);
}


@end

*/