//
//  RamdiskBuilderOSXAppDelegate.h
//  RamdiskBuilderOSX
//
//  Created by msftguy on 5/21/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface RamdiskBuilderOSXAppDelegate : NSObject <NSApplicationDelegate> {
    NSWindow *window;
}

@property (assign) IBOutlet NSWindow *window;

@end
