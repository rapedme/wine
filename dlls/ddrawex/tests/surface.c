/*
 * Unit tests for ddrawex surfaces
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#define COBJMACROS
/* For IID_IDirectDraw3 - it is not in dxguid.dll */
#define INITGUID

#include <assert.h>
#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "ddraw.h"
#include "ddrawex.h"
#include "unknwn.h"

static IDirectDrawFactory *factory;
static HRESULT (WINAPI *pDllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID *ppv);

static IDirectDraw *createDD(void)
{
    HRESULT hr;
    IDirectDraw *dd;

    hr = IDirectDrawFactory_CreateDirectDraw(factory, NULL, NULL, DDSCL_NORMAL, 0,
                                             0, &dd);
    ok(hr == DD_OK, "Failed to create an IDirectDraw interface, hr = 0x%08x\n", hr);
    return SUCCEEDED(hr) ? dd : NULL;
}

static void dctest_surf(IDirectDrawSurface *surf, int ddsdver) {
    HRESULT hr;
    HDC dc, dc2 = (HDC) 0x1234;
    DDSURFACEDESC ddsd;
    DDSURFACEDESC2 ddsd2;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);

    hr = IDirectDrawSurface_GetDC(surf, &dc);
    ok(hr == DD_OK, "IDirectDrawSurface_GetDC failed: 0x%08x\n", hr);

    hr = IDirectDrawSurface_GetDC(surf, &dc);
    ok(hr == DDERR_DCALREADYCREATED, "IDirectDrawSurface_GetDC failed: 0x%08x\n", hr);
    ok(dc2 == (HDC) 0x1234, "The failed GetDC call changed the dc: %p\n", dc2);

    hr = IDirectDrawSurface_Lock(surf, NULL, ddsdver == 1 ? &ddsd : ((DDSURFACEDESC *) &ddsd2), 0, NULL);
    ok(hr == DDERR_SURFACEBUSY, "IDirectDrawSurface_Lock returned 0x%08x, expected DDERR_SURFACEBUSY\n", hr);

    hr = IDirectDrawSurface_ReleaseDC(surf, dc);
    ok(hr == DD_OK, "IDirectDrawSurface_ReleaseDC failed: 0x%08x\n", hr);
    hr = IDirectDrawSurface_ReleaseDC(surf, dc);
    ok(hr == DDERR_NODC, "IDirectDrawSurface_ReleaseDC returned 0x%08x, expected DDERR_NODC\n", hr);
}

static void GetDCTest_main(DDSURFACEDESC *ddsd, DDSURFACEDESC2 *ddsd2, void (*testfunc)(IDirectDrawSurface *surf, int ddsdver))
{
    IDirectDrawSurface *surf;
    IDirectDrawSurface2 *surf2;
    IDirectDrawSurface2 *surf3;
    IDirectDrawSurface4 *surf4;
    HRESULT hr;
    IDirectDraw  *dd1 = createDD();
    IDirectDraw2 *dd2;
    IDirectDraw3 *dd3;
    IDirectDraw4 *dd4;

    hr = IDirectDraw_CreateSurface(dd1, ddsd, &surf, NULL);
    ok(hr == DD_OK, "IDirectDraw_CreateSurface failed: 0x%08x\n", hr);
    testfunc(surf, 1);
    IDirectDrawSurface_Release(surf);

    hr = IDirectDraw_QueryInterface(dd1, &IID_IDirectDraw2, (void **) &dd2);
    ok(hr == DD_OK, "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    hr = IDirectDraw2_CreateSurface(dd2, ddsd, &surf, NULL);
    ok(hr == DD_OK, "IDirectDraw2_CreateSurface failed: 0x%08x\n", hr);
    testfunc(surf, 1);

    hr = IDirectDrawSurface_QueryInterface(surf, &IID_IDirectDrawSurface2, (void **) &surf2);
    ok(hr == DD_OK, "IDirectDrawSurface_QueryInterface failed: 0x%08x\n", hr);
    testfunc((IDirectDrawSurface *) surf2, 1);

    IDirectDrawSurface2_Release(surf2);
    IDirectDrawSurface_Release(surf);
    IDirectDraw2_Release(dd2);

    hr = IDirectDraw_QueryInterface(dd1, &IID_IDirectDraw3, (void **) &dd3);
    ok(hr == DD_OK, "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    hr = IDirectDraw3_CreateSurface(dd3, ddsd, &surf, NULL);
    ok(hr == DD_OK, "IDirectDraw3_CreateSurface failed: 0x%08x\n", hr);
    testfunc(surf, 1);

    hr = IDirectDrawSurface_QueryInterface(surf, &IID_IDirectDrawSurface3, (void **) &surf3);
    ok(hr == DD_OK, "IDirectDrawSurface_QueryInterface failed: 0x%08x\n", hr);
    testfunc((IDirectDrawSurface *) surf3, 1);

    IDirectDrawSurface3_Release(surf3);
    IDirectDrawSurface_Release(surf);
    IDirectDraw3_Release(dd3);

    hr = IDirectDraw_QueryInterface(dd1, &IID_IDirectDraw4, (void **) &dd4);
    ok(hr == DD_OK, "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    surf = NULL;
    hr = IDirectDraw4_CreateSurface(dd4, ddsd2, &surf4, NULL);
    ok(hr == DD_OK, "IDirectDraw4_CreateSurface failed: 0x%08x\n", hr);
    testfunc((IDirectDrawSurface *) surf4, 2);

    IDirectDrawSurface4_Release(surf4);
    IDirectDraw4_Release(dd4);

    IDirectDraw_Release(dd1);
}

static void GetDCTest(void)
{
    DDSURFACEDESC ddsd;
    DDSURFACEDESC2 ddsd2;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = 64;
    ddsd.dwHeight = 64;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    ddsd2.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd2.dwWidth = 64;
    ddsd2.dwHeight = 64;
    ddsd2.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

    GetDCTest_main(&ddsd, &ddsd2, dctest_surf);
}

static void CapsTest(void)
{
    DDSURFACEDESC ddsd;
    IDirectDraw  *dd1 = createDD();
    IDirectDrawSurface *surf;
    HRESULT hr;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_VIDEOMEMORY;
    ddsd.dwWidth = 64;
    ddsd.dwHeight = 64;
    hr = IDirectDraw_CreateSurface(dd1, &ddsd, &surf, NULL);
    ok(hr == DD_OK, "Creating a SYSMEM | VIDMEM surface returned 0x%08x, expected DD_OK\n", hr);
    if(surf) IDirectDrawSurface_Release(surf);

    IDirectDraw_Release(dd1);
}

static void dctest_sysvidmem(IDirectDrawSurface *surf, int ddsdver) {
    HRESULT hr;
    HDC dc, dc2 = (HDC) 0x1234;
    DDSURFACEDESC ddsd;
    DDSURFACEDESC2 ddsd2;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);

    hr = IDirectDrawSurface_GetDC(surf, &dc);
    ok(hr == DD_OK, "IDirectDrawSurface_GetDC failed: 0x%08x\n", hr);

    hr = IDirectDrawSurface_GetDC(surf, &dc2);
    ok(hr == DD_OK, "IDirectDrawSurface_GetDC failed: 0x%08x\n", hr);
    ok(dc == dc2, "Got two different DCs\n");

    hr = IDirectDrawSurface_Lock(surf, NULL, ddsdver == 1 ? &ddsd : ((DDSURFACEDESC *) &ddsd2), 0, NULL);
    ok(hr == DD_OK, "IDirectDrawSurface_Lock returned 0x%08x, expected DD_OK\n", hr);

    hr = IDirectDrawSurface_Lock(surf, NULL, ddsdver == 1 ? &ddsd : ((DDSURFACEDESC *) &ddsd2), 0, NULL);
    ok(hr == DDERR_SURFACEBUSY, "IDirectDrawSurface_Lock returned 0x%08x, expected DDERR_SURFACEBUSY\n", hr);

    hr = IDirectDrawSurface_Unlock(surf, NULL);
    ok(hr == DD_OK, "IDirectDrawSurface_Unlock returned 0x%08x, expected DD_OK\n", hr);
    hr = IDirectDrawSurface_Unlock(surf, NULL);
    ok(hr == DDERR_NOTLOCKED, "IDirectDrawSurface_Unlock returned 0x%08x, expected DDERR_NOTLOCKED\n", hr);

    hr = IDirectDrawSurface_ReleaseDC(surf, dc);
    ok(hr == DD_OK, "IDirectDrawSurface_ReleaseDC failed: 0x%08x\n", hr);
    hr = IDirectDrawSurface_ReleaseDC(surf, dc);
    ok(hr == DD_OK, "IDirectDrawSurface_ReleaseDC failed: 0x%08x\n", hr);
    /* That works any number of times... */
    hr = IDirectDrawSurface_ReleaseDC(surf, dc);
    ok(hr == DD_OK, "IDirectDrawSurface_ReleaseDC failed: 0x%08x\n", hr);
}

static void SysVidMemTest(void)
{
    DDSURFACEDESC ddsd;
    DDSURFACEDESC2 ddsd2;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = 64;
    ddsd.dwHeight = 64;
    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_VIDEOMEMORY;
    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    ddsd2.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd2.dwWidth = 64;
    ddsd2.dwHeight = 64;
    ddsd2.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_VIDEOMEMORY;

    GetDCTest_main(&ddsd, &ddsd2, dctest_sysvidmem);
}

START_TEST(surface)
{
    IClassFactory *classfactory = NULL;
    ULONG ref;
    HRESULT hr;
    HMODULE hmod = LoadLibrary("ddrawex.dll");
    if(hmod == NULL) {
        skip("Failed to load ddrawex.dll\n");
        return;
    }
    pDllGetClassObject = (void*)GetProcAddress(hmod, "DllGetClassObject");
    if(pDllGetClassObject == NULL) {
        skip("Failed to get DllGetClassObject\n");
        return;
    }

    hr = pDllGetClassObject(&CLSID_DirectDrawFactory, &IID_IClassFactory, (void **) &classfactory);
    ok(hr == S_OK, "Failed to create a IClassFactory\n");
    hr = IClassFactory_CreateInstance(classfactory, NULL, &IID_IDirectDrawFactory, (void **) &factory);
    ok(hr == S_OK, "Failed to create a IDirectDrawFactory\n");

    GetDCTest();
    CapsTest();
    SysVidMemTest();

    if(factory) {
        ref = IDirectDrawFactory_Release(factory);
        ok(ref == 0, "IDirectDrawFactory not cleanly released\n");
    }
    if(classfactory) {
        ref = IClassFactory_Release(classfactory);
        todo_wine ok(ref == 1, "IClassFactory refcount wrong, ref = %u\n", ref);
    }
}
