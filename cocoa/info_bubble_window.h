// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

// A rounded window with an arrow used for example when you click on the STAR
// button or that pops up within our first-run UI.
@interface InfoBubbleWindow : NSWindow {
 @private
  // Is self in the process of closing.
  BOOL closing_;
  // If NO the window will close immediately instead of fading out.
  // Default YES.
  BOOL delayOnClose_;
}

// Returns YES if the window is in the process of closing.
// Can't use "windowWillClose" notification because that will be sent
// after the closing animation has completed.
- (BOOL)isClosing;

@property BOOL delayOnClose;

@end
