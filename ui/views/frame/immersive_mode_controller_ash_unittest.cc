// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/immersive_mode_controller_ash.h"

#include "ash/test/ash_test_base.h"
#include "chrome/browser/ui/immersive_fullscreen_configuration.h"
#include "ui/aura/client/cursor_client.h"
#include "ui/aura/root_window.h"
#include "ui/aura/test/event_generator.h"
#include "ui/aura/window.h"
#include "ui/views/bubble/bubble_delegate.h"

// For now, immersive fullscreen is Chrome OS only.
#if defined(OS_CHROMEOS)

/////////////////////////////////////////////////////////////////////////////

class MockImmersiveModeControllerDelegate
    : public ImmersiveModeController::Delegate {
 public:
  MockImmersiveModeControllerDelegate() : immersive_style_(false) {}
  virtual ~MockImmersiveModeControllerDelegate() {}

  bool immersive_style() const { return immersive_style_; }

  // ImmersiveModeController::Delegate overrides:
  virtual BookmarkBarView* GetBookmarkBar() OVERRIDE { return NULL; }
  virtual FullscreenController* GetFullscreenController() OVERRIDE {
    return NULL;
  }
  virtual void FullscreenStateChanged() OVERRIDE {}
  virtual void SetImmersiveStyle(bool immersive) OVERRIDE {
    immersive_style_ = immersive;
  }
  virtual content::WebContents* GetWebContents() OVERRIDE {
    return NULL;
  }

 private:
  bool immersive_style_;

  DISALLOW_COPY_AND_ASSIGN(MockImmersiveModeControllerDelegate);
};

/////////////////////////////////////////////////////////////////////////////

class ImmersiveModeControllerAshTest : public ash::test::AshTestBase {
 public:
  enum Modality {
    MODALITY_MOUSE,
    MODALITY_TOUCH,
    MODALITY_GESTURE
  };

  ImmersiveModeControllerAshTest() : widget_(NULL), top_container_(NULL) {}
  virtual ~ImmersiveModeControllerAshTest() {}

  ImmersiveModeControllerAsh* controller() { return controller_.get(); }
  views::View* top_container() { return top_container_; }
  MockImmersiveModeControllerDelegate* delegate() { return delegate_.get(); }

  aura::test::EventGenerator* event_generator() {
    return event_generator_.get();
  }

  // Access to private data from the controller.
  bool top_edge_hover_timer_running() const {
    return controller_->top_edge_hover_timer_.IsRunning();
  }
  int mouse_x_when_hit_top() const {
    return controller_->mouse_x_when_hit_top_;
  }

  // ash::test::AshTestBase overrides:
  virtual void SetUp() OVERRIDE {
    ash::test::AshTestBase::SetUp();

    ImmersiveFullscreenConfiguration::EnableImmersiveFullscreenForTest();
    ASSERT_TRUE(ImmersiveFullscreenConfiguration::UseImmersiveFullscreen());

    controller_.reset(new ImmersiveModeControllerAsh);
    delegate_.reset(new MockImmersiveModeControllerDelegate);

    event_generator_.reset(new aura::test::EventGenerator(CurrentContext()));

    widget_ = new views::Widget();
    views::Widget::InitParams params;
    params.context = CurrentContext();
    params.bounds = gfx::Rect(0, 0, 500, 500);
    widget_->Init(params);
    widget_->Show();

    top_container_ = new views::View();
    top_container_->SetBounds(0, 0, 500, 100);
    top_container_->set_focusable(true);

    widget_->GetContentsView()->AddChildView(top_container_);

    controller_->Init(delegate_.get(), widget_, top_container_);
    controller_->DisableAnimationsForTest();
  }

  // Attempt to reveal the top-of-window views via |modality|.
  // The top-of-window views can only be revealed via mouse hover or a gesture.
  void AttemptReveal(Modality modality) {
    ASSERT_NE(modality, MODALITY_TOUCH);
    AttemptRevealStateChange(true, modality);
  }

  // Attempt to unreveal the top-of-window views via |modality|. The
  // top-of-window views can be unrevealed via any modality.
  void AttemptUnreveal(Modality modality) {
    AttemptRevealStateChange(false, modality);
  }

  // Sets whether the mouse is hovered above |top_container_|.
  // SetHovered(true) moves the mouse over the |top_container_| but does not
  // move it to the top of the screen so will not initiate a reveal.
  void SetHovered(bool is_mouse_hovered) {
    MoveMouse(0, is_mouse_hovered ? 1 : top_container_->height() + 100);
  }

  // Move the mouse to the given coordinates. The coordinates should be in
  // |top_container_| coordinates.
  void MoveMouse(int x, int y) {
    // Luckily, |top_container_| is at the top left of the root window so the
    // provided coordinates are already in the coordinates of the root window.
    event_generator_->MoveMouseTo(x, y);

    // If the top edge timer started running as a result of the mouse move, run
    // the task which occurs after the timer delay. This reveals the
    // top-of-window views synchronously if the mouse is hovered at the top of
    // the screen.
    if (controller()->top_edge_hover_timer_.IsRunning()) {
      controller()->top_edge_hover_timer_.user_task().Run();
      controller()->top_edge_hover_timer_.Stop();
    }
  }

 private:
  // Attempt to change the revealed state to |revealed| via |modality|.
  void AttemptRevealStateChange(bool revealed, Modality modality) {
    // Compute the event position in |top_container_| coordinates.
    gfx::Point event_position(0, revealed ? 0 : top_container_->height() + 100);
    switch (modality) {
      case MODALITY_MOUSE: {
        MoveMouse(event_position.x(), event_position.y());
        break;
      }
      case MODALITY_TOUCH: {
        // Luckily, |top_container_| is at the top left of the root window so
        // |event_position| is already in the coordinates of the root window.
        event_generator_->MoveTouch(event_position);
        event_generator_->PressTouch();
        event_generator_->ReleaseTouch();
        break;
      }
      case MODALITY_GESTURE: {
        aura::client::GetCursorClient(CurrentContext())->DisableMouseEvents();
        ImmersiveModeControllerAsh::SwipeType swipe_type = revealed ?
            ImmersiveModeControllerAsh::SWIPE_OPEN :
            ImmersiveModeControllerAsh::SWIPE_CLOSE;
        controller_->UpdateRevealedLocksForSwipe(swipe_type);
        break;
      }
    }
  }

  scoped_ptr<ImmersiveModeControllerAsh> controller_;
  scoped_ptr<MockImmersiveModeControllerDelegate> delegate_;
  views::Widget* widget_;  // Owned by the native widget.
  views::View* top_container_;  // Owned by |root_view_|.
  scoped_ptr<aura::test::EventGenerator> event_generator_;

  DISALLOW_COPY_AND_ASSIGN(ImmersiveModeControllerAshTest);
};

// Test of initial state and basic functionality.
TEST_F(ImmersiveModeControllerAshTest, ImmersiveModeControllerAsh) {
  // Initial state.
  EXPECT_FALSE(controller()->IsEnabled());
  EXPECT_FALSE(controller()->ShouldHideTopViews());
  EXPECT_FALSE(controller()->IsRevealed());
  EXPECT_FALSE(delegate()->immersive_style());

  // Enabling hides the top views.
  controller()->SetEnabled(true);
  EXPECT_TRUE(controller()->IsEnabled());
  EXPECT_FALSE(controller()->IsRevealed());
  EXPECT_TRUE(controller()->ShouldHideTopViews());
  EXPECT_FALSE(controller()->ShouldHideTabIndicators());
  EXPECT_TRUE(delegate()->immersive_style());

  // Revealing shows the top views.
  AttemptReveal(MODALITY_MOUSE);
  EXPECT_TRUE(controller()->IsRevealed());
  EXPECT_FALSE(controller()->ShouldHideTopViews());
  // Tabs are painting in the normal style during a reveal.
  EXPECT_FALSE(delegate()->immersive_style());
}

// Test mouse event processing for top-of-screen reveal triggering.
TEST_F(ImmersiveModeControllerAshTest, OnMouseEvent) {
  // Set up initial state.
  controller()->SetEnabled(true);
  ASSERT_TRUE(controller()->IsEnabled());
  ASSERT_FALSE(controller()->IsRevealed());

  // Mouse wheel event does nothing.
  ui::MouseEvent wheel(
      ui::ET_MOUSEWHEEL, gfx::Point(), gfx::Point(), ui::EF_NONE);
  event_generator()->Dispatch(&wheel);
  EXPECT_FALSE(top_edge_hover_timer_running());

  // Move to top edge of screen starts hover timer running. We cannot use
  // MoveMouse() because MoveMouse() stops the timer if it started running.
  event_generator()->MoveMouseTo(100, 0);
  EXPECT_TRUE(top_edge_hover_timer_running());
  EXPECT_EQ(100, mouse_x_when_hit_top());

  // Moving off the top edge stops it.
  event_generator()->MoveMouseTo(100, 1);
  EXPECT_FALSE(top_edge_hover_timer_running());

  // Moving back to the top starts the timer again.
  event_generator()->MoveMouseTo(100, 0);
  EXPECT_TRUE(top_edge_hover_timer_running());
  EXPECT_EQ(100, mouse_x_when_hit_top());

  // Slight move to the right keeps the timer running for the same hit point.
  event_generator()->MoveMouseTo(101, 0);
  EXPECT_TRUE(top_edge_hover_timer_running());
  EXPECT_EQ(100, mouse_x_when_hit_top());

  // Moving back to the left also keeps the timer running.
  event_generator()->MoveMouseTo(100, 0);
  EXPECT_TRUE(top_edge_hover_timer_running());
  EXPECT_EQ(100, mouse_x_when_hit_top());

  // Large move right restarts the timer (so it is still running) and considers
  // this a new hit at the top.
  event_generator()->MoveMouseTo(499, 0);
  EXPECT_TRUE(top_edge_hover_timer_running());
  EXPECT_EQ(499, mouse_x_when_hit_top());

  // Moving off the top edge horizontally stops the timer.
  EXPECT_GT(CurrentContext()->bounds().width(), top_container()->width());
  EXPECT_EQ(500, top_container()->width());
  event_generator()->MoveMouseTo(500, 0);
  EXPECT_FALSE(top_edge_hover_timer_running());

  // Once revealed, a move just a little below the top container doesn't end a
  // reveal.
  AttemptReveal(MODALITY_MOUSE);
  event_generator()->MoveMouseTo(0, top_container()->height() + 1);
  EXPECT_TRUE(controller()->IsRevealed());

  // Once revealed, clicking just below the top container ends the reveal.
  event_generator()->ClickLeftButton();
  EXPECT_FALSE(controller()->IsRevealed());

  // Moving a lot below the top container ends a reveal.
  AttemptReveal(MODALITY_MOUSE);
  EXPECT_TRUE(controller()->IsRevealed());
  event_generator()->MoveMouseTo(0, top_container()->height() + 50);
  EXPECT_FALSE(controller()->IsRevealed());

  // The mouse position cannot cause a reveal when TopContainerView's widget
  // has capture.
  views::Widget* widget = top_container()->GetWidget();
  widget->SetCapture(top_container());
  AttemptReveal(MODALITY_MOUSE);
  EXPECT_FALSE(controller()->IsRevealed());
  widget->ReleaseCapture();

  // The mouse position cannot end the reveal while TopContainerView's widget
  // has capture.
  AttemptReveal(MODALITY_MOUSE);
  EXPECT_TRUE(controller()->IsRevealed());
  widget->SetCapture(top_container());
  event_generator()->MoveMouseTo(0, top_container()->height() + 51);
  EXPECT_TRUE(controller()->IsRevealed());

  // Releasing capture should end the reveal.
  widget->ReleaseCapture();
  EXPECT_FALSE(controller()->IsRevealed());
}

// Test that hovering the mouse over the find bar does not end a reveal.
TEST_F(ImmersiveModeControllerAshTest, FindBar) {
  // Set up initial state.
  controller()->SetEnabled(true);
  ASSERT_TRUE(controller()->IsEnabled());
  ASSERT_FALSE(controller()->IsRevealed());

  // Compute the find bar bounds relative to TopContainerView. The find
  // bar is aligned with the bottom right of the TopContainerView.
  gfx::Rect find_bar_bounds(top_container()->bounds().right() - 100,
                            top_container()->bounds().bottom(),
                            100,
                            50);

  gfx::Point find_bar_position_in_screen = find_bar_bounds.origin();
  views::View::ConvertPointToScreen(top_container(),
      &find_bar_position_in_screen);
  gfx::Rect find_bar_bounds_in_screen(find_bar_position_in_screen,
      find_bar_bounds.size());
  controller()->OnFindBarVisibleBoundsChanged(find_bar_bounds_in_screen);

  // Moving the mouse over the find bar does not end the reveal.
  gfx::Point over_find_bar(find_bar_bounds.x() + 25, find_bar_bounds.y() + 25);
  AttemptReveal(MODALITY_MOUSE);
  EXPECT_TRUE(controller()->IsRevealed());
  MoveMouse(over_find_bar.x(), over_find_bar.y());
  EXPECT_TRUE(controller()->IsRevealed());

  // Moving the mouse off of the find bar horizontally ends the reveal.
  MoveMouse(find_bar_bounds.x() - 25, find_bar_bounds.y() + 25);
  EXPECT_FALSE(controller()->IsRevealed());

  // Moving the mouse off of the find bar vertically ends the reveal.
  AttemptReveal(MODALITY_MOUSE);
  EXPECT_TRUE(controller()->IsRevealed());
  MoveMouse(find_bar_bounds.x() + 25, find_bar_bounds.bottom() + 25);

  // Similar to the TopContainerView, moving the mouse slightly off vertically
  // of the find bar does not end the reveal.
  AttemptReveal(MODALITY_MOUSE);
  MoveMouse(find_bar_bounds.x() + 25, find_bar_bounds.bottom() + 1);
  EXPECT_TRUE(controller()->IsRevealed());

  // Similar to the TopContainerView, clicking the mouse even slightly off of
  // the find bar ends the reveal.
  event_generator()->ClickLeftButton();
  EXPECT_FALSE(controller()->IsRevealed());

  // Set the find bar bounds to empty. Hovering over the position previously
  // occupied by the find bar, |over_find_bar|, should end the reveal.
  controller()->OnFindBarVisibleBoundsChanged(gfx::Rect());
  AttemptReveal(MODALITY_MOUSE);
  MoveMouse(over_find_bar.x(), over_find_bar.y());
  EXPECT_FALSE(controller()->IsRevealed());
}

// Test revealing the top-of-window views using one modality and ending
// the reveal via another. For instance, initiating the reveal via a SWIPE_OPEN
// edge gesture, switching to using the mouse and ending the reveal by moving
// the mouse off of the top-of-window views.
TEST_F(ImmersiveModeControllerAshTest, DifferentModalityEnterExit) {
  controller()->SetEnabled(true);
  EXPECT_TRUE(controller()->IsEnabled());
  EXPECT_FALSE(controller()->IsRevealed());

  // Initiate reveal via gesture, end reveal via mouse.
  AttemptReveal(MODALITY_GESTURE);
  EXPECT_TRUE(controller()->IsRevealed());
  MoveMouse(1, 1);
  EXPECT_TRUE(controller()->IsRevealed());
  AttemptUnreveal(MODALITY_MOUSE);
  EXPECT_FALSE(controller()->IsRevealed());

  // Initiate reveal via gesture, end reveal via touch.
  AttemptReveal(MODALITY_GESTURE);
  EXPECT_TRUE(controller()->IsRevealed());
  AttemptUnreveal(MODALITY_TOUCH);
  EXPECT_FALSE(controller()->IsRevealed());

  // Initiate reveal via mouse, end reveal via gesture.
  AttemptReveal(MODALITY_MOUSE);
  EXPECT_TRUE(controller()->IsRevealed());
  AttemptUnreveal(MODALITY_GESTURE);
  EXPECT_FALSE(controller()->IsRevealed());

  // Initiate reveal via mouse, end reveal via touch.
  AttemptReveal(MODALITY_MOUSE);
  EXPECT_TRUE(controller()->IsRevealed());
  AttemptUnreveal(MODALITY_TOUCH);
  EXPECT_FALSE(controller()->IsRevealed());
}

// Test when the SWIPE_CLOSE edge gesture closes the top-of-window views.
TEST_F(ImmersiveModeControllerAshTest, EndRevealViaGesture) {
  controller()->SetEnabled(true);
  EXPECT_TRUE(controller()->IsEnabled());
  EXPECT_FALSE(controller()->IsRevealed());

  // A gesture should be able to close the top-of-window views when
  // top-of-window views have focus.
  AttemptReveal(MODALITY_MOUSE);
  top_container()->RequestFocus();
  EXPECT_TRUE(controller()->IsRevealed());
  AttemptUnreveal(MODALITY_GESTURE);
  EXPECT_FALSE(controller()->IsRevealed());
  top_container()->GetFocusManager()->ClearFocus();

  // If some other code is holding onto a lock, a gesture should not be able to
  // end the reveal.
  AttemptReveal(MODALITY_MOUSE);
  scoped_ptr<ImmersiveRevealedLock> lock(controller()->GetRevealedLock(
      ImmersiveModeController::ANIMATE_REVEAL_NO));
  EXPECT_TRUE(controller()->IsRevealed());
  AttemptUnreveal(MODALITY_GESTURE);
  EXPECT_TRUE(controller()->IsRevealed());
  lock.reset();
  EXPECT_FALSE(controller()->IsRevealed());
}

// Do not test under windows because focus testing is not reliable on
// Windows. (crbug.com/79493)
#if !defined(OS_WIN)

// Test how focus and activation affects whether the top-of-window views are
// revealed.
TEST_F(ImmersiveModeControllerAshTest, Focus) {
  // Add views to the view hierarchy which we will focus and unfocus during the
  // test.
  views::View* child_view = new views::View();
  child_view->SetBounds(0, 0, 10, 10);
  child_view->set_focusable(true);
  top_container()->AddChildView(child_view);
  views::View* unrelated_view = new views::View();
  unrelated_view->SetBounds(0, 100, 10, 10);
  unrelated_view->set_focusable(true);
  top_container()->parent()->AddChildView(unrelated_view);
  views::FocusManager* focus_manager =
      top_container()->GetWidget()->GetFocusManager();

  controller()->SetEnabled(true);

  // 1) Test that the top-of-window views stay revealed as long as either a
  // |child_view| has focus or the mouse is hovered above the top-of-window
  // views.
  AttemptReveal(MODALITY_MOUSE);
  child_view->RequestFocus();
  focus_manager->ClearFocus();
  EXPECT_TRUE(controller()->IsRevealed());
  child_view->RequestFocus();
  SetHovered(false);
  EXPECT_TRUE(controller()->IsRevealed());
  focus_manager->ClearFocus();
  EXPECT_FALSE(controller()->IsRevealed());

  // 2) Test that focusing |unrelated_view| hides the top-of-window views.
  // Note: In this test we can cheat and trigger a reveal via focus because
  // the top container does not hide when the top-of-window views are not
  // revealed.
  child_view->RequestFocus();
  EXPECT_TRUE(controller()->IsRevealed());
  unrelated_view->RequestFocus();
  EXPECT_FALSE(controller()->IsRevealed());

  // 3) Test that a loss of focus of |child_view| to |unrelated_view|
  // while immersive mode is disabled is properly registered.
  child_view->RequestFocus();
  EXPECT_TRUE(controller()->IsRevealed());
  controller()->SetEnabled(false);
  EXPECT_FALSE(controller()->IsRevealed());
  unrelated_view->RequestFocus();
  controller()->SetEnabled(true);
  EXPECT_FALSE(controller()->IsRevealed());

  // Repeat test but with a revealed lock acquired when immersive mode is
  // disabled because the code path is different.
  child_view->RequestFocus();
  EXPECT_TRUE(controller()->IsRevealed());
  controller()->SetEnabled(false);
  scoped_ptr<ImmersiveRevealedLock> lock(controller()->GetRevealedLock(
      ImmersiveModeController::ANIMATE_REVEAL_NO));
  EXPECT_FALSE(controller()->IsRevealed());
  unrelated_view->RequestFocus();
  controller()->SetEnabled(true);
  EXPECT_TRUE(controller()->IsRevealed());
  lock.reset();
  EXPECT_FALSE(controller()->IsRevealed());
}

// Test how activation affects whether the top-of-window views are revealed.
// The behavior when a bubble is activated is tested in
// ImmersiveModeControllerAshTest.Bubbles.
TEST_F(ImmersiveModeControllerAshTest, Activation) {
  views::Widget* top_container_widget = top_container()->GetWidget();

  controller()->SetEnabled(true);
  ASSERT_FALSE(controller()->IsRevealed());

  // 1) Test that a transient window which is not a bubble does not trigger a
  // reveal but does keep the top-of-window views revealed if they are already
  // revealed.
  views::Widget::InitParams transient_params;
  transient_params.ownership =
      views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  transient_params.parent = top_container_widget->GetNativeView();
  transient_params.bounds = gfx::Rect(0, 0, 100, 100);
  scoped_ptr<views::Widget> transient_widget(new views::Widget());
  transient_widget->Init(transient_params);
  transient_widget->Show();

  EXPECT_FALSE(controller()->IsRevealed());
  top_container_widget->Activate();
  AttemptReveal(MODALITY_MOUSE);
  EXPECT_TRUE(controller()->IsRevealed());
  transient_widget->Activate();
  SetHovered(false);
  EXPECT_TRUE(controller()->IsRevealed());
  transient_widget.reset();
  EXPECT_FALSE(controller()->IsRevealed());

  // 2) Test that activating a non-transient window ends the reveal if any.
  views::Widget::InitParams non_transient_params;
  non_transient_params.ownership =
      views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  non_transient_params.context = top_container_widget->GetNativeView();
  non_transient_params.bounds = gfx::Rect(0, 0, 100, 100);
  scoped_ptr<views::Widget> non_transient_widget(new views::Widget());
  non_transient_widget->Init(non_transient_params);
  non_transient_widget->Show();

  EXPECT_FALSE(controller()->IsRevealed());
  top_container_widget->Activate();
  AttemptReveal(MODALITY_MOUSE);
  EXPECT_TRUE(controller()->IsRevealed());
  non_transient_widget->Activate();
  EXPECT_FALSE(controller()->IsRevealed());
}

// Test how bubbles affect whether the top-of-window views are revealed.
TEST_F(ImmersiveModeControllerAshTest, Bubbles) {
  scoped_ptr<ImmersiveRevealedLock> revealed_lock;
  views::Widget* top_container_widget = top_container()->GetWidget();

  // Add views to the view hierarchy to which we will anchor bubbles.
  views::View* child_view = new views::View();
  child_view->SetBounds(0, 0, 10, 10);
  top_container()->AddChildView(child_view);
  views::View* unrelated_view = new views::View();
  unrelated_view->SetBounds(0, 100, 10, 10);
  top_container()->parent()->AddChildView(unrelated_view);

  controller()->SetEnabled(true);
  ASSERT_FALSE(controller()->IsRevealed());

  // 1) Test that a bubble anchored to a child of the top container triggers
  // a reveal and keeps the top-of-window views revealed for the duration of
  // its visibility.
  views::Widget* bubble_widget1(views::BubbleDelegateView::CreateBubble(
      new views::BubbleDelegateView(child_view, views::BubbleBorder::NONE)));
  bubble_widget1->Show();
  EXPECT_TRUE(controller()->IsRevealed());

  // Activating |top_container_widget| will close |bubble_widget1|.
  top_container_widget->Activate();
  AttemptReveal(MODALITY_MOUSE);
  revealed_lock.reset(controller()->GetRevealedLock(
      ImmersiveModeController::ANIMATE_REVEAL_NO));
  EXPECT_TRUE(controller()->IsRevealed());

  views::Widget* bubble_widget2 = views::BubbleDelegateView::CreateBubble(
      new views::BubbleDelegateView(child_view, views::BubbleBorder::NONE));
  bubble_widget2->Show();
  EXPECT_TRUE(controller()->IsRevealed());
  revealed_lock.reset();
  SetHovered(false);
  EXPECT_TRUE(controller()->IsRevealed());
  bubble_widget2->Close();
  EXPECT_FALSE(controller()->IsRevealed());

  // 2) Test that the top-of-window views stay revealed as long as at least one
  // bubble anchored to a child of the top container is visible.
  views::BubbleDelegateView* bubble_delegate3(new views::BubbleDelegateView(
      child_view, views::BubbleBorder::NONE));
  bubble_delegate3->set_use_focusless(true);
  views::Widget* bubble_widget3(views::BubbleDelegateView::CreateBubble(
      bubble_delegate3));
  bubble_widget3->Show();

  views::BubbleDelegateView* bubble_delegate4(new views::BubbleDelegateView(
      child_view, views::BubbleBorder::NONE));
  bubble_delegate4->set_use_focusless(true);
  views::Widget* bubble_widget4(views::BubbleDelegateView::CreateBubble(
      bubble_delegate4));
  bubble_widget4->Show();

  EXPECT_TRUE(controller()->IsRevealed());
  bubble_widget3->Hide();
  EXPECT_TRUE(controller()->IsRevealed());
  bubble_widget4->Hide();
  EXPECT_FALSE(controller()->IsRevealed());
  bubble_widget4->Show();
  EXPECT_TRUE(controller()->IsRevealed());

  // 3) Test that visibility changes which occur while immersive fullscreen is
  // disabled are handled upon reenabling immersive fullscreen.
  controller()->SetEnabled(false);
  bubble_widget4->Hide();
  controller()->SetEnabled(true);
  EXPECT_FALSE(controller()->IsRevealed());

  // We do not need |bubble_widget3| or |bubble_widget4| anymore, close them.
  bubble_widget3->Close();
  bubble_widget4->Close();

  // 4) Test that a bubble added while immersive fullscreen is disabled is
  // handled upon reenabling immersive fullscreen.
  controller()->SetEnabled(false);

  views::Widget* bubble_widget5 = views::BubbleDelegateView::CreateBubble(
      new views::BubbleDelegateView(child_view, views::BubbleBorder::NONE));
  bubble_widget5->Show();

  controller()->SetEnabled(true);
  EXPECT_TRUE(controller()->IsRevealed());

  bubble_widget5->Close();

  // 5) Test that a bubble which is not anchored to a child of the
  // TopContainerView does not trigger a reveal or keep the
  // top-of-window views revealed if they are already revealed.
  views::Widget* bubble_widget6 = views::BubbleDelegateView::CreateBubble(
      new views::BubbleDelegateView(unrelated_view, views::BubbleBorder::NONE));
  bubble_widget6->Show();
  EXPECT_FALSE(controller()->IsRevealed());

  // Activating |top_container_widget| will close |bubble_widget6|.
  top_container_widget->Activate();
  AttemptReveal(MODALITY_MOUSE);
  EXPECT_TRUE(controller()->IsRevealed());

  views::Widget* bubble_widget7 = views::BubbleDelegateView::CreateBubble(
      new views::BubbleDelegateView(unrelated_view, views::BubbleBorder::NONE));
  bubble_widget7->Show();
  SetHovered(false);
  EXPECT_FALSE(controller()->IsRevealed());
  bubble_widget7->Close();
}

#endif  // defined(OS_WIN)

#endif  // defined(OS_CHROMEOS)
