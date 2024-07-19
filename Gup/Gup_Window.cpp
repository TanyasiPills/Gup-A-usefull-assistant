#include "Window.h"
#include <Windows.h>
#include <windows.h>
#include <d2d1.h>
#include <iostream>
#include <tchar.h>
#include <wincodec.h>
#include <vector>
#include <thread>
#include <mutex>
#include <random>
#include <gdiplus.h>
#include <string>

#pragma comment (lib,"Gdiplus.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "windowscodecs.lib")

using namespace std;
using namespace Gdiplus;

///////////
//Structs//
///////////
struct PoinTF {
    float x;
    float y;
};

/////////////
//Variables//
/////////////

//main window
const wchar_t* CLASS_NAME = L"DUCK";
HINSTANCE hi;
HWND mainWindow;
int screenWidth;
int screenHeight;
int halfScreenWidth;
int halfScreenHeight;

//random shit
mutex mtx;
random_device rnd;
mt19937 gen(rnd());

//d2d1
ID2D1Factory* pD2DFactory = NULL;
IWICImagingFactory* pIWICFactory = NULL;
ID2D1HwndRenderTarget* pRenderTarget = NULL;
void Initialize2D(HWND hWnd);
void CleanUp2D();

//rsources/images
vector<ID2D1Bitmap*> images;
vector<ID2D1Bitmap*> images_fliped;
ID2D1Bitmap* image;
D2D1_SIZE_U imageSize;
D2D1_SIZE_U halfImageSize;
int image_count = 0;

//render
thread image_render;
thread UpdateLoc;
bool render = true;

//calculations
POINT gupPos = {-1200,0};
POINT target;
PoinTF orientation;

//gaussian
const UINT c = 7;
const UINT a = 4;
const float o = 0.3f;
const float v = 0.001f;
const float odo = 1 / o * a;;

//mouse lock
int counter = 0;
bool hooked = false;
POINT randP;
HHOOK g_hMouseHook = NULL;
uniform_int_distribution<int> dist(-10, 10);


//movement pathfinging
bool reached = false;
bool done = false;
int behaviour = 1;
uniform_int_distribution<int> behav(0, 2);
uniform_int_distribution<int> loc(2, 8);
uniform_int_distribution<int> cordX;
uniform_int_distribution<int> cordY;
vector<POINT*> cords;
int cordCount;

//meme window
HANDLE memethread;
HINSTANCE memeInstance;
HWND memes_hwnd;
uniform_int_distribution<int> meme(0, 7);
bool moving;
POINT memeSize;
DWORD meme_style = SWP_NOSIZE | SWP_NOZORDER;
int memeY;
int memeCount;
vector<Bitmap*> memes;
static Gdiplus::GdiplusStartupInput gdiplusStartupInput;
static ULONG_PTR gdiplusToken;

//memory management
template <class T>
inline void SafeRelease(T** ppT)
{
    if (*ppT) {
        (*ppT)->Release();
        *ppT = nullptr;
    }
}

//////////////////////
//Not main callbacks//
//////////////////////
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        if (wParam == WM_MOUSEMOVE) {
            return 1;
        }
    }
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}
LRESULT CALLBACK MemeWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_PAINT:
    {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            Graphics nyas(hdc);
            nyas.DrawImage(memes[memeCount], 0, 0);
            EndPaint(hwnd, &ps);
        
        return 0;
    }
    case WM_SYSCOMMAND:
        if (!done) return 0;
        if (wParam == SC_SIZE) return 0;
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

////////////////
//Meme windows//
////////////////
void UpdateMemeWindow() {
    while (moving) {
        SetWindowPos(memes_hwnd, NULL, gupPos.x + halfScreenWidth - memeSize.x - halfImageSize.width,halfScreenHeight - gupPos.y - memeSize.y - halfImageSize.height, 0, 0, meme_style);
        this_thread::sleep_for(chrono::milliseconds(36));
    }
}
void CreateMemeWindow(HINSTANCE m_hInstance) {
    const wchar_t* CLASS_NAME = L"Meme Class";

    memeCount = meme(gen);

    WNDCLASS wndClass = {};
    wndClass.lpszClassName = CLASS_NAME;
    wndClass.hInstance = m_hInstance;
    wndClass.hIcon = LoadIcon(NULL, MAKEINTRESOURCE(192));
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.lpfnWndProc = MemeWindowProc;

    RegisterClass(&wndClass);

    memeSize.x = memes[memeCount]->GetWidth();
    memeSize.y = memes[memeCount]->GetHeight();
    memeSize.x += 10;
    memeSize.y += 30;

    DWORD meme_st = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;

    memes_hwnd = CreateWindowEx(WS_EX_TOPMOST, CLASS_NAME, L"Meme :3", meme_st, -1000, -1000, memeSize.x, memeSize.y, NULL, NULL, m_hInstance, NULL);

    memeSize.y /= 2;
    memeSize.x -= 50;
    memeSize.y -= 50;
    moving = true;
    memeY = cordY(gen);

    SetWindowPos(memes_hwnd, HWND_TOP, -1000, -1000, 0, 0, meme_style);
    SetWindowPos(mainWindow, memes_hwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    ShowWindow(memes_hwnd, SW_SHOW);
    UpdateWindow(memes_hwnd);

    thread UpdMeme(UpdateMemeWindow);
    UpdMeme.detach();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    };
}
unsigned int __stdcall ThreadProc(void* lpParameter)
{
    CreateMemeWindow(memeInstance);
    return 0;
}

//////////////////////
//Funciton functions//
//////////////////////
void Reached() {
    if (behaviour == 2) {
        if (cordCount + 1 == cords.size()) {
            done = true;
            behaviour = 0;
            return;
        }
        cordCount++;
    }
    else {
        
        if (reached == true && gupPos.x > -100) {
            done = true;
            moving = false;
            behaviour = 0;
            SetWindowPos(mainWindow, memes_hwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            return;
        }
        reached = true;
        memethread = (HANDLE)_beginthreadex(NULL, 0, &ThreadProc, NULL, 0, NULL);
    }
}
void Update() {
    mtx.lock();

    switch (behaviour) {
        case 0:
            GetCursorPos(&target);
            ScreenToClient(mainWindow, &target);
            break;
        case 1:
            if (reached) {
                target.x = halfScreenWidth;
                target.y = memeY;
                break;
            }
            target.x = -200;
            target.y = halfScreenHeight;
            break;
        case 2:
            target = *cords[cordCount];
    }
    target.x -= halfScreenWidth;
    target.y = halfScreenHeight - target.y;
    float distX = target.x - gupPos.x;
    float distY = target.y - gupPos.y;
    int distMax = abs(distX) + abs(distY);

    if (distX != 0 || distY != 0) {
        orientation.x = distX / distMax;
        orientation.y = distY / distMax;
    }

    if (behaviour != 0 && distMax < 100) Reached();

    mtx.unlock();
}
int Gaussian() {
    return odo * exp(-pow((image_count - c) / o, 2) * v);
}
void CheckMouse() {
    while (true) {
        mtx.lock();
        if (counter > 0) {
            counter--;
        }
        else {
            GetCursorPos(&target);
            ScreenToClient(mainWindow, &target);
            target.x -= halfScreenWidth;
            target.y = halfScreenHeight - target.y;
            int distMax = abs(target.x - gupPos.x) + abs(target.y - gupPos.y);

            if (distMax < 60 && counter == 0 && !hooked) {
                g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);
                randP.x = dist(gen);
                randP.y = dist(gen);
                hooked = true;
                counter += 35;

                MSG msg;
                while (PeekMessage(&msg, mainWindow, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            else if (counter == 0) {
                UnhookWindowsHookEx(g_hMouseHook);
                counter += 10;
                hooked = false;
            }
        }
        mtx.unlock();
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}
void CurPosSet() {
    while (true) {
        while (hooked)
        {
            SetCursorPos(gupPos.x + halfScreenWidth + randP.x, halfScreenHeight - gupPos.y + randP.y);
        }
        this_thread::sleep_for(chrono::milliseconds(36));
    }
}
void Update_Behv() {
    while (true) {
        if (done) {

            behaviour = behav(gen);

            if (behaviour == 2) {
                cords.resize(loc(gen));
                for (POINT*& item : cords) {
                    if (item != nullptr) delete item;
                    item = new POINT{ cordX(gen), cordY(gen) };
                }
            }

            done = false;
        }
        this_thread::sleep_for(chrono::milliseconds(60000));
    }
}

////////////////////
//Resource loading//
////////////////////
ID2D1Bitmap* LoadResourceBitmap(HINSTANCE hi, int index)
{
    IWICBitmapDecoder* pDecoder = NULL;
    IWICBitmapFrameDecode* pSource = NULL;
    IWICStream* pStream = NULL;
    IWICFormatConverter* pConverter = NULL;

    HRSRC imageResHandle = NULL;
    HGLOBAL imageResDataHandle = NULL;
    void* pImageFile = NULL;
    DWORD imageFileSize = 0;

    D2D1_BITMAP_PROPERTIES bitmapProperties = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    imageResHandle = FindResource(hi, MAKEINTRESOURCE(index), _T("PNG"));
    HRESULT hr = imageResHandle ? S_OK : E_FAIL;

    if (SUCCEEDED(hr))
    {
        imageResDataHandle = LoadResource(hi, imageResHandle);
        hr = imageResDataHandle ? S_OK : E_FAIL;
    }
    if (SUCCEEDED(hr))
    {
        pImageFile = LockResource(imageResDataHandle);
        hr = pImageFile ? S_OK : E_FAIL;
    }
    if (SUCCEEDED(hr))
    {
        imageFileSize = SizeofResource(hi, imageResHandle);
        hr = imageFileSize ? S_OK : E_FAIL;
    }
    if (SUCCEEDED(hr))hr = pIWICFactory->CreateStream(&pStream);
    if (SUCCEEDED(hr)) hr = pStream->InitializeFromMemory(reinterpret_cast<BYTE*>(pImageFile),imageFileSize);
    if (SUCCEEDED(hr)) hr = pIWICFactory->CreateDecoderFromStream(pStream,NULL,WICDecodeMetadataCacheOnLoad,&pDecoder);
    if (SUCCEEDED(hr)) hr = pDecoder->GetFrame(0, &pSource);
    if (SUCCEEDED(hr)) hr = pIWICFactory->CreateFormatConverter(&pConverter);
    if (SUCCEEDED(hr)) hr = pConverter->Initialize(pSource,GUID_WICPixelFormat32bppPBGRA,WICBitmapDitherTypeNone,NULL,0.f,WICBitmapPaletteTypeMedianCut);
    if (SUCCEEDED(hr)) hr = pRenderTarget->CreateBitmapFromWicBitmap(pConverter, bitmapProperties,&image);

    SafeRelease(&pDecoder);
    SafeRelease(&pSource);
    SafeRelease(&pStream);
    SafeRelease(&pConverter);

    return image;
}
Bitmap* LoadBitmapFromResource(HINSTANCE hInstance, LPCTSTR resourceName) {
    HRSRC hResource = FindResource(hInstance, resourceName, _T("PNG"));
    HGLOBAL hMemory = LoadResource(hInstance, hResource);
    DWORD imageSize = SizeofResource(hInstance, hResource);
    void* pResourceData = LockResource(hMemory);
    HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, imageSize);

    void* pBuffer = GlobalLock(hBuffer);
    if (!pBuffer) {
        GlobalFree(hBuffer);
        return nullptr;
    }

    CopyMemory(pBuffer, pResourceData, imageSize);
    GlobalUnlock(hBuffer);

    IStream* pStream = nullptr;
    if (CreateStreamOnHGlobal(hBuffer, TRUE, &pStream) != S_OK) {
        GlobalFree(hBuffer);
        return nullptr;
    }

    Bitmap* pBitmap = new Bitmap(pStream, FALSE);
    pStream->Release();
    return pBitmap;
}

/////////////////
//Gup rendering//
/////////////////
void RenderImage() {
    thread LockMouse(CurPosSet);
    LockMouse.detach();
    thread LookForMouse(CheckMouse);
    LookForMouse.detach();
    thread BehaviourUpd(Update_Behv);
    BehaviourUpd.detach();

    while (true) {
        mtx.lock();
        if (image_count == 28) {
            image_count = 0;
            thread UpdateLoc(Update);
            UpdateLoc.detach();
        }

        int gausy = Gaussian();
        gupPos.x += gausy * orientation.x;
        gupPos.y += gausy * orientation.y;
        if (orientation.x < 0) image = images_fliped[image_count];
        else image = images[image_count];
        image_count++;
        render = true;
        mtx.unlock();
        this_thread::sleep_for(chrono::milliseconds(36));
    }
}
void Render() {
    if (!pRenderTarget) {return;}

    pRenderTarget->BeginDraw();
    pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));

    if (image) {
        int x = gupPos.x + halfScreenWidth - halfImageSize.width;
        int y = halfScreenHeight - gupPos.y - halfImageSize.height;

        if (((x <= 0 && x > -static_cast<int>(imageSize.width)) || (x >= screenWidth && x < (screenWidth + imageSize.width))) || ((y <= 0 && y > -static_cast<int>(imageSize.height)) || (y >= screenHeight && y < (screenHeight+imageSize.height)))) {
            D2D1_SIZE_U fakeSize;
            D2D1_RECT_U srcRect;
            int xD = 0;
            int yD = 0;
            int xU = 0;
            int yU = 0;

            if (x < 0) {
                xD = abs(x);
                x = 0;
            }
            else if (x > screenWidth) {
                xU = x - screenWidth;
                x = screenWidth;
            }
            if (y < 0) {
                yD = abs(y);
                y = 0;
            }
            else if (y > screenHeight) {
                yU = y - screenHeight;
                y = screenHeight;
            }

            srcRect = D2D1::RectU(0 + xD, 0 + yD, imageSize.width - xU, imageSize.height - yU);
            D2D1_SIZE_F destSize = D2D1::SizeF(srcRect.right - srcRect.left, srcRect.bottom - srcRect.top);

            ID2D1BitmapRenderTarget* pBitmapRenderTarget = nullptr;
            pRenderTarget->CreateCompatibleRenderTarget(destSize, &pBitmapRenderTarget);

            ID2D1Bitmap* destinationImage = nullptr;
            pBitmapRenderTarget->GetBitmap(&destinationImage);

            D2D1_POINT_2U destPoint = D2D1::Point2U(0, 0);
            destinationImage->CopyFromBitmap(&destPoint, image, &srcRect);
            fakeSize = destinationImage->GetPixelSize();

            pRenderTarget->DrawBitmap(destinationImage, D2D1::RectF(x, y, x + fakeSize.width, y + fakeSize.height));

        }
        else if (x > 0 && x < screenWidth && y > 0 && y < screenHeight) pRenderTarget->DrawBitmap(image, D2D1::RectF(x, y, x + imageSize.width, y + imageSize.height));
    }
    pRenderTarget->EndDraw();
}

////////////////////////
//Main window callback//
////////////////////////
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_CREATE:
        {
            halfScreenWidth = screenWidth / 2;
            halfScreenHeight = screenHeight / 2;
            cordX = uniform_int_distribution<int>(100, screenWidth - 100);
            cordY = uniform_int_distribution<int>(100, screenHeight - 100);
            return 0;
        }
        case WM_CLOSE:
            DestroyWindow(hWnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_PAINT:
            if (render) {
                Render();
                render = false;
            }
            return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//////////////////////////////
//Window generation/demolish//
//////////////////////////////
Window::Window() {
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    WNDCLASS wndClass = {};
    wndClass.lpszClassName = CLASS_NAME;
    wndClass.hInstance = m_hInstance;
    wndClass.hIcon = LoadIcon(NULL, MAKEINTRESOURCE(192));
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.lpfnWndProc = WindowProc;

    hi = m_hInstance;

    RegisterClass(&wndClass);

    screenWidth = GetSystemMetrics(SM_CXSCREEN);
    screenHeight = GetSystemMetrics(SM_CYSCREEN);

    DWORD style2 = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT;
    DWORD style3 = WS_POPUP;

    m_hWnd = CreateWindowEx(style2, CLASS_NAME, L"Ajtó", style3, 0, 0, screenWidth, screenHeight, NULL, NULL, m_hInstance, NULL);

    SetLayeredWindowAttributes(m_hWnd, RGB(0, 0, 0), 255, LWA_COLORKEY);

    Initialize2D(m_hWnd);

    mainWindow = m_hWnd;

    for (size_t i = 158; i < 166; i++) memes.push_back(LoadBitmapFromResource(GetModuleHandle(NULL), MAKEINTRESOURCE(i)));

    image_render = thread(RenderImage);

    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);
}
Window::~Window() {
    UnregisterClass(CLASS_NAME, m_hInstance);
    delete CLASS_NAME;
    delete pD2DFactory;
    delete pIWICFactory;
    delete pRenderTarget;
    for (ID2D1Bitmap* bitik : images) bitik->Release();
    for (ID2D1Bitmap* bitik : images_fliped) bitik->Release();
    delete image;
    CleanUp2D();
}
bool Window::ProcessMessages()
{
    MSG msg = {};

    while (PeekMessage(&msg, nullptr, 0u, 0u, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return true;
}

//////////////////////
//Render stuff - gpu//
//////////////////////
void Initialize2D(HWND hWnd) {
    D2D1_FACTORY_OPTIONS options;
    options.debugLevel = D2D1_DEBUG_LEVEL_NONE;

    HRESULT pp = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,__uuidof(ID2D1Factory),&options,reinterpret_cast<void**>(&pD2DFactory));
    
    RECT rc;
    GetClientRect(hWnd, &rc);

    pp = pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_HARDWARE), D2D1::HwndRenderTargetProperties(hWnd, D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top)), &pRenderTarget);

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr)) hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*)&pIWICFactory);

    for (size_t i = 101; i < 129; i++) images_fliped.push_back(LoadResourceBitmap(hi, i));
    for (size_t i = 130; i < 158; i++) images.push_back(LoadResourceBitmap(hi, i));
    imageSize = images[0]->GetPixelSize();
    halfImageSize.width = imageSize.width / 2;
    halfImageSize.height = imageSize.height / 2;
    render = false;
}
void CleanUp2D()
{
    SafeRelease(&pRenderTarget);
    SafeRelease(&pD2DFactory);
    SafeRelease(&pIWICFactory);
    CoUninitialize();
}