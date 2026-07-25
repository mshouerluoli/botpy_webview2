#pragma once

#include <windows.h>
#include <WebView2.h>
#include <string>
#include "MainWindow.h"

class CreateEnvironmentCompletedHandler : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
{
private:
    MainWindow* m_window;
    long m_refCount = 1;

public:
    CreateEnvironmentCompletedHandler(MainWindow* window) : m_window(window) {}

    HRESULT STDMETHODCALLTYPE Invoke(HRESULT errorCode, ICoreWebView2Environment* environment) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppvObj) override;
};

class CreateControllerCompletedHandler : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler
{
private:
    MainWindow* m_window;
    long m_refCount = 1;

public:
    CreateControllerCompletedHandler(MainWindow* window) : m_window(window) {}

    HRESULT STDMETHODCALLTYPE Invoke(HRESULT errorCode, ICoreWebView2Controller* controller) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppvObj) override;
};

class WebMessageHandler : public ICoreWebView2WebMessageReceivedEventHandler
{
private:
    MainWindow* m_window;
    long m_refCount = 1;

public:
    WebMessageHandler(MainWindow* window) : m_window(window), m_refCount(1) {}

    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppvObj) override;

    HRESULT STDMETHODCALLTYPE Invoke(
        ICoreWebView2* sender,
        ICoreWebView2WebMessageReceivedEventArgs* args) override;
};

class NavigationCompletedHandler : public ICoreWebView2NavigationCompletedEventHandler
{
private:
    MainWindow* m_window;
    long m_refCount = 1;

public:
    NavigationCompletedHandler(MainWindow* window) : m_window(window) {}

    HRESULT STDMETHODCALLTYPE Invoke(
        ICoreWebView2* sender,
        ICoreWebView2NavigationCompletedEventArgs* args) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppvObj) override;
};
