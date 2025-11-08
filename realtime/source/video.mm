#import "merc/av/realtime/video.h"
#import "merc/av/realtime/video-impl.h"
#import "merc/av/thread.h"
#import "merc/vst/graphics.h"
#import "merc/global/config.h"

@interface MercView : MTKView
@end

@implementation MercView
- (void)keyDown:(NSEvent*)event
{
    switch (event.keyCode)
    {
    case 53: // esc
        [self.window close];
        return;
    case 48: // tab (passed up to switch editor and merc window for now)
        [[self nextResponder] keyDown:event];
        return;
    }
}
- (BOOL)acceptsFirstResponder
{
    return YES;
}
@end

@interface MercVideo : NSObject <NSWindowDelegate, MTKViewDelegate>
- (id)initWithRevolver:(std::shared_ptr<merc::av::Revolver<merc::av::Pixels>>)r
               factory:(merc::vst::Factory&)factory;
- (BOOL)isActive;
@end

@implementation MercVideo
{
    BOOL active;
    std::shared_ptr<merc::av::Revolver<merc::av::Pixels>> revolver;
    id<MTLCommandQueue> commandQueue;
    id<MTLRenderPipelineState> pipelineState;
}

- (id)initWithRevolver:(std::shared_ptr<merc::av::Revolver<merc::av::Pixels>>)r
               factory:(merc::vst::Factory&)factory
{
    self = [super init];
    if (!self) return nil;
    active = true;
    revolver = std::move(r);
    commandQueue = factory.graphics->commandQueue;
    return self;
}

- (void)windowWillClose:(NSNotification *)notification
{
    active = false;
}

- (BOOL)isActive
{
    return active;
}

- (void)drawInMTKView:(nonnull MTKView*)view
{
    auto shot{ revolver->fire() };
    if (shot.blank) return;
    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
    commandBuffer.label = @"MercVideoDrawInViewCommandBuffer";
    auto blitEncoder = [commandBuffer blitCommandEncoder];
    auto currentDrawable = view.currentDrawable;
    if (currentDrawable == nil) return;
    [blitEncoder copyFromTexture:shot.bullet->texture->mtlTexture toTexture:currentDrawable.texture];
    [blitEncoder endEncoding];
    [commandBuffer presentDrawable:view.currentDrawable];
    [commandBuffer commit];
}

- (void)mtkView:(nonnull MTKView*)view drawableSizeWillChange:(CGSize)size
{

}
@end

namespace merc::av::realtime
{
    VideoImplementation::VideoImplementation(Out& mainOut, vst::Factory& factory)
    {
        window = [NSWindow.alloc initWithContentRect: NSMakeRect(0, 0, global::getConfig().video.width, global::getConfig().video.height)
                                           styleMask: NSWindowStyleMaskTitled | NSWindowStyleMaskFullSizeContentView
                                             backing: NSBackingStoreBuffered
                                               defer: false];
        window.titlebarAppearsTransparent = true;
        window.movableByWindowBackground = true;
        view = [[MercView alloc] initWithFrame:[window contentLayoutRect]
                                        device:factory.graphics->device];
        view.paused = true;
        view.enableSetNeedsDisplay = false;
        view.framebufferOnly = false;
        realtimeVideo = [[MercVideo alloc] initWithRevolver:std::get<PixelRevolver>(mainOut.revolvers)
                                                    factory:factory];
        window.delegate = realtimeVideo;
        view.delegate = realtimeVideo;
        [window setFrameOrigin: NSMakePoint(global::getConfig().video.x, global::getConfig().video.y)];
        [window setContentView:view];
        [window makeKeyAndOrderFront: window];
        window.releasedWhenClosed = false;
    }

    VideoImplementation::operator bool() const
    {
        return [realtimeVideo isActive];
    }

    Video::Video(Out& mainOut, vst::Factory& factory)
        : implementation{ std::make_shared<VideoImplementation>(mainOut, factory) }
        , realtimeThread
        {
            VideoThread
            {
                implementation,
                &realtimeThreadCancelled,
                std::chrono::duration<double, std::milli>{ 1.0 / global::getConfig().video.framesPerSecond }
            }
        }
    {}
    Video::~Video()
    {
        realtimeThreadCancelled.test_and_set();
        realtimeThread.join();
    }
    Video::operator bool() const { return *implementation; }

    void VideoThread::operator()()
    {
        raiseCurrentThreadPriority();
        auto frameStartTime{ std::chrono::steady_clock::now() };
        while (!cancelled->test(std::memory_order_relaxed))
        {
            [implementation->view draw];
            const auto frameCompleteTime{ std::chrono::steady_clock::now() };
            const auto sleepTime{ timePerFrame - (frameCompleteTime - frameStartTime) };
            if (sleepTime.count() > 0.0) std::this_thread::sleep_for(sleepTime);
            frameStartTime = std::chrono::steady_clock::now();
        }
    }
}
