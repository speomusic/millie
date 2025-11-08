//#include "merc/editor.h"
//#include "chain-impl.h"
//#include "merc/vst-utils.h"
//#include "merc/windows-utils.h"
//#include <pluginterfaces/gui/iplugview.h>
//#include <public.sdk/source/vst/utility/memoryibstream.h>
//#include <windows.h>
//
//namespace merc
//{
//    struct EditorWindow : Steinberg::IPlugFrame
//    {
//        EditorWindow(Steinberg::IPtr<Steinberg::IPlugView> v);
//        ~EditorWindow();
//        operator bool() const;
//        void onDestroy();
//        const int topMargin{ 15 };
//        DECLARE_FUNKNOWN_METHODS
//    private:
//        Steinberg::IPtr<Steinberg::IPlugView> plugView;
//        HWND parentWindow, pluginWindow;
//        bool active{ true };
//        int getPluginWindowWidth() const;
//        int getPluginWindowHeight() const;
//        Steinberg::tresult PLUGIN_API resizeView(Steinberg::IPlugView* view, Steinberg::ViewRect* newSize) override;
//    };
//
//    Editor::Editor(const Steinberg::IPtr<Steinberg::Vst::IComponent>& component,
//                   std::shared_ptr<ChainImpl> c,
//                   Space& space)
//        : chain{ std::move(c) }
//        , editController{ makeEditController(component, chain.get(), space) }
//        , editControllerIsSeparatePluginBase{ !Steinberg::FUnknownPtr<Steinberg::Vst::IEditController>(component) }
//        , window{ std::make_unique<EditorWindow>(editController->createView("editor")) }
//    {
//        CHECK_TRESULT(editController->setComponentHandler(this));
//        Steinberg::ResizableMemoryIBStream stream;
//        CHECK_TRESULT(component->getState(&stream));
//        stream.rewind();
//        CHECK_TRESULT(editController->setComponentState(&stream));
//    }
//
//    Editor::~Editor()
//    {
//        if (editControllerIsSeparatePluginBase)
//            editController->terminate();
//    }
//
//    Editor::operator bool() const
//    {
//        return *window;
//    }
//
//    static LRESULT CALLBACK ParentWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
//    {
//        if (message == WM_CREATE)
//        {
//            auto createStruct{ reinterpret_cast<LPCREATESTRUCT>(lParam) };
//            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
//        }
//
//        if (auto editorWindow{ reinterpret_cast<EditorWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA)) })
//        {
//            switch (message)
//            {
//            case WM_CREATE:
//                // Hide windows frame: https://learn.microsoft.com/en-us/windows/win32/dwm/customframe
//                RECT r; GetWindowRect(hWnd, &r);
//                SetWindowPos(hWnd, nullptr, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_FRAMECHANGED);
//                break;
//            case WM_PAINT:
//            {
//                PAINTSTRUCT ps;
//                auto hdc{ BeginPaint(hWnd, &ps) };
//                ps.rcPaint.top = ps.rcPaint.top + 2;
//                ps.rcPaint.bottom = ps.rcPaint.top + 1;
//                FillRect(hdc, &ps.rcPaint, (HBRUSH)GetStockObject(DKGRAY_BRUSH));
//                EndPaint(hWnd, &ps);
//                break;
//            }
//            case WM_NCCALCSIZE:
//                // hide default window frame
//                if (wParam == TRUE) return 0;
//                break;
//            case WM_NCHITTEST:
//                // every left click moves the window
//                return HTCAPTION;
//            case WM_KEYDOWN:
//                switch (wParam)
//                {
//                case 27: // esc
//                    DestroyWindow(hWnd);
//                    break;
//                }
//                break;
//            case WM_DESTROY:
//                editorWindow->onDestroy();
//                break;
//            }
//        }
//
//        return DefWindowProc(hWnd, message, wParam, lParam);
//    }
//
//    static LPCWSTR registerParentWindowClass(HINSTANCE hInstance)
//    {
//        LPCWSTR ret{ 0 };
//        WNDCLASS windowClass
//        {
//            .style = CS_HREDRAW | CS_VREDRAW,
//            .lpfnWndProc = ParentWindowProc,
//            .hInstance = hInstance,
//            .hCursor = LoadCursor(NULL, IDC_ARROW),
//            .hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH),
//            .lpszClassName = L"mercEditorWindowClass"
//        };
//        return MAKEINTATOM(RegisterClass(&windowClass));
//    }
//
//    static LPCWSTR registerPluginWindowClass(HINSTANCE hInstance)
//    {
//        LPCWSTR ret{ 0 };
//        WNDCLASS windowClass
//        {
//            .style = CS_HREDRAW | CS_VREDRAW,
//            .lpfnWndProc = DefWindowProc,
//            .hInstance = hInstance,
//            .hCursor = LoadCursor(NULL, IDC_ARROW),
//            .hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH),
//            .lpszClassName = L"mercEditorPluginWindowClass"
//        };
//        return MAKEINTATOM(RegisterClass(&windowClass));
//    }
//
//    EditorWindow::EditorWindow(Steinberg::IPtr<Steinberg::IPlugView> v)
//        : plugView{ std::move(v) }
//        , parentWindow
//        {
//            CreateWindowEx(
//                0,
//                registerParentWindowClass(getCurrentModule()),
//                nullptr,
//                WS_OVERLAPPEDWINDOW,
//                CW_USEDEFAULT,
//                CW_USEDEFAULT,
//                getPluginWindowWidth(),
//                getPluginWindowHeight() + topMargin,
//                nullptr, 
//                nullptr,
//                getCurrentModule(),
//                this)
//        }
//        , pluginWindow
//        {
//            CreateWindowEx(
//                0,
//                registerPluginWindowClass(getCurrentModule()),
//                nullptr,
//                WS_CHILDWINDOW,
//                0,
//                topMargin,
//                getPluginWindowWidth(),
//                getPluginWindowHeight(),
//                parentWindow, 
//                nullptr,
//                getCurrentModule(),
//                this)
//        }
//    {
//        CHECK_TRESULT(plugView->setFrame(this));
//        CHECK_TRESULT_IMPLEMENTED(plugView->isPlatformTypeSupported(Steinberg::kPlatformTypeHWND));
//        CHECK_TRESULT(plugView->attached((void*)pluginWindow, Steinberg::kPlatformTypeHWND));
//
//        ShowWindow(parentWindow, SW_NORMAL);
//        ShowWindow(pluginWindow, SW_NORMAL);
//    }
//
//    EditorWindow::~EditorWindow()
//    {
//        if (active)
//        {
//            DestroyWindow(pluginWindow);
//            DestroyWindow(parentWindow);
//        }
//    }
//
//    EditorWindow::operator bool() const
//    {
//        return active;
//    }
//
//    void EditorWindow::onDestroy()
//    {
//        plugView->removed();
//        active = false;
//    }
//
//    int EditorWindow::getPluginWindowWidth() const
//    {
//        Steinberg::ViewRect size;
//        CHECK_TRESULT(plugView->getSize(&size));
//        return size.getWidth();
//    }
//
//    int EditorWindow::getPluginWindowHeight() const
//    {
//        Steinberg::ViewRect size;
//        CHECK_TRESULT(plugView->getSize(&size));
//        return size.getHeight();
//    }
//
//    Steinberg::tresult EditorWindow::resizeView(Steinberg::IPlugView* view, Steinberg::ViewRect* newSize)
//    {
//        RECT current;
//        if (!GetWindowRect(parentWindow, &current)) THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
//        if (!MoveWindow(parentWindow, current.left, current.top, newSize->getWidth(), newSize->getHeight() + topMargin, true)) THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
//        if (!MoveWindow(pluginWindow, 0, topMargin, newSize->getWidth(), newSize->getHeight(), false)) THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
//        view->onSize(newSize);
//        return Steinberg::kResultOk;
//    }
//
//    Steinberg::tresult PLUGIN_API EditorWindow::queryInterface(const char* _iid, void** obj)
//    {
//        return Steinberg::kNotImplemented;
//    }
//
//    Steinberg::uint32 PLUGIN_API EditorWindow::addRef()
//    {
//        return 1;
//    }
//
//    Steinberg::uint32 PLUGIN_API EditorWindow::release()
//    {
//        return 1;
//    }
//}
