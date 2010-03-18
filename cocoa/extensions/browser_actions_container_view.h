// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COCOA_EXTENSIONS_BROWSER_ACTIONS_CONTAINER_VIEW_
#define CHROME_BROWSER_COCOA_EXTENSIONS_BROWSER_ACTIONS_CONTAINER_VIEW_

#import <Cocoa/Cocoa.h>

// Sent when a user-initiated drag to resize the container is initiated.
extern const NSString* kBrowserActionGrippyDragStartedNotification;

// Sent when a user-initiated drag is resizing the container.
extern const NSString* kBrowserActionGrippyDraggingNotification;

// Sent when a user-initiated drag to resize the container has finished.
extern const NSString* kBrowserActionGrippyDragFinishedNotification;

// The view that encompasses the Browser Action buttons in the toolbar and
// provides mechanisms for resizing.
@interface BrowserActionsContainerView : NSView {
 @private
  // The frame encompasing the grippy used for resizing the container.
  NSRect grippyRect_;

  // Used to cache the original position within the container that initiated the
  // drag.
  NSPoint initialDragPoint_;

  // Used to cache the previous x-pos of the frame rect for resizing purposes.
  CGFloat lastXPos_;

  // Whether there is a border to the right of the last Browser Action.
  BOOL rightBorderShown_;

  // Whether the container is currently being resized by the user.
  BOOL userIsResizing_;

  // Whether the user is allowed to drag the grippy to the left. NO if all
  // extensions are shown or the location bar has hit its minimum width (handled
  // within toolbar_controller.mm).
  BOOL canDragLeft_;

  // Whether the user is allowed to drag the grippy to the right. NO if all
  // extensions are hidden.
  BOOL canDragRight_;

  // When the left grippy is pinned, resizing the window has no effect on its
  // position. This prevents it from overlapping with other elements as well
  // as letting the container expand when the window is going from super small
  // to large.
  BOOL grippyPinned_;
}

// Resizes the container to the given ideal width, adjusting the |lastXPos_| so
// that |resizeDeltaX| is accurate.
- (void)resizeToWidth:(CGFloat)width animate:(BOOL)animate;

// Returns the change in the x-pos of the frame rect during resizing. Meant to
// be queried when a NSViewFrameDidChangeNotification is fired to determine
// placement of surrounding elements.
- (CGFloat)resizeDeltaX;

@property(nonatomic) BOOL canDragLeft;
@property(nonatomic) BOOL canDragRight;
@property(nonatomic) BOOL grippyPinned;
@property(readonly, nonatomic) BOOL userIsResizing;
@property(nonatomic) BOOL rightBorderShown;

@end

#endif  // CHROME_BROWSER_COCOA_EXTENSIONS_BROWSER_ACTIONS_CONTAINER_VIEW_
