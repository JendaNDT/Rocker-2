#include "Devices.h"
#if JUCE_WINDOWS
#include <mmdeviceapi.h>
#include <propsys.h>
#include <wrl/client.h>
#endif
namespace rocker {
#if JUCE_WINDOWS
namespace {
using Microsoft::WRL::ComPtr;
std::optional<GUID> containerFor(EDataFlow flow, const juce::String& name) {
    ComPtr<IMMDeviceEnumerator> enumerator;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                IID_PPV_ARGS(&enumerator))))
        return {};
    ComPtr<IMMDeviceCollection> devices;
    if (FAILED(enumerator->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &devices)))
        return {};
    UINT n = 0;
    devices->GetCount(&n);
    const PROPERTYKEY friendly{
        {0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}}, 14};
    const PROPERTYKEY container{
        {0x8c7ed206, 0x3f8a, 0x4827, {0xb3, 0xab, 0xae, 0x9e, 0x1f, 0xae, 0xfc, 0x6c}}, 2};
    for (UINT i = 0; i < n; ++i) {
        ComPtr<IMMDevice> device;
        ComPtr<IPropertyStore> properties;
        if (FAILED(devices->Item(i, &device)) ||
            FAILED(device->OpenPropertyStore(STGM_READ, &properties)))
            continue;
        PROPVARIANT label{};
        properties->GetValue(friendly, &label);
        bool match = label.vt == VT_LPWSTR && name == juce::String(label.pwszVal);
        PropVariantClear(&label);
        if (!match)
            continue;
        PROPVARIANT id{};
        properties->GetValue(container, &id);
        std::optional<GUID> result;
        if (id.vt == VT_CLSID && id.puuid && !IsEqualGUID(*id.puuid, GUID_NULL))
            result = *id.puuid;
        PropVariantClear(&id);
        return result;
    }
    return {};
}
} // namespace
#endif
bool separateWindowsClocks(const DeviceChoice& choice) {
    if (choice.driver != Driver::exclusive && choice.driver != Driver::lowLatency &&
        choice.driver != Driver::shared)
        return false;
    if (choice.input.isEmpty() || choice.output.isEmpty() || choice.input == choice.output)
        return false;
#if JUCE_WINDOWS
    auto in = containerFor(eCapture, choice.input), out = containerFor(eRender, choice.output);
    if (in && out)
        return !IsEqualGUID(*in, *out);
#endif
    return true;
}
} // namespace rocker
