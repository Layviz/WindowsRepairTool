#include "DesktopNotificationManagerCompat.h"
#include <NotificationActivationCallback.h>
#include <windows.ui.notifications.h>
#include "notification.h"

using namespace ABI::Windows::Data::Xml::Dom;
using namespace ABI::Windows::UI::Notifications;
using namespace Microsoft::WRL;


// The UUID CLSID must be unique to your app. Create a new GUID if copying this code.
class DECLSPEC_UUID("C4DA8B01-4175-4F0E-888B-FFEAE726598B") NotificationActivator WrlSealed WrlFinal
    : public RuntimeClass<RuntimeClassFlags<ClassicCom>, INotificationActivationCallback>
{
public:
    virtual HRESULT STDMETHODCALLTYPE Activate(
        _In_ LPCWSTR appUserModelId,
        _In_ LPCWSTR invokedArgs,
        _In_reads_(dataCount) const NOTIFICATION_USER_INPUT_DATA * data,
        ULONG dataCount) override
    {
        HRESULT hr = S_OK;
        // TODO: Handle activation

        return S_OK;
    }
};

// Flag class as COM creatable
CoCreatableClass(NotificationActivator);

HRESULT register_notification_activator() {
    HRESULT hr;
    // Register AUMID and COM server (for a packaged app, this is a no-operation)
    hr = DesktopNotificationManagerCompat::RegisterAumidAndComServer(L"Layviz.WindowsReperaturTool", __uuidof(NotificationActivator));
    if (SUCCEEDED(hr)) {
        // Register activator type
        hr = DesktopNotificationManagerCompat::RegisterActivator();
        if (!SUCCEEDED(hr)) {
            printf("RegisterActivator failed\n");
        }
    }
    else {
        printf("RegisterAumidAndComServer failed\n");
    }
    return hr;
}

HRESULT send_notification(std::wstring string) {
    HRESULT hr;

    std::wstring xml = L"<toast><visual><binding template='ToastGeneric'><text>" + string + L"</text></binding></visual></toast>";
    // Construct XML
    ComPtr<IXmlDocument> doc;
    hr = DesktopNotificationManagerCompat::CreateXmlDocumentFromString(
        L"<toast><visual><binding template='ToastGeneric'><text>Hello world</text></binding></visual></toast>",
        &doc);
    if (SUCCEEDED(hr))
    {
        // See full code sample to learn how to inject dynamic text, buttons, and more

        // Create the notifier
        // Desktop apps must use the compat method to create the notifier.
        ComPtr<IToastNotifier> notifier;
        hr = DesktopNotificationManagerCompat::CreateToastNotifier(&notifier);
        if (SUCCEEDED(hr))
        {
            // Create the notification itself (using helper method from compat library)
            ComPtr<IToastNotification> toast;
            hr = DesktopNotificationManagerCompat::CreateToastNotification(doc.Get(), &toast);
            if (SUCCEEDED(hr))
            {
                // And show it!
                hr = notifier->Show(toast.Get());
            }
            else {
                printf("CreateToastNotification failed\n");
            }
        }
        else {
            printf("CreateToastNotifier failed\n");
        }
    }
    else {
        printf("CreateXmlDocumentFromString failed\n");
    }
    return hr;
}