## RegisterHotKey

For hotkeys on Windows, we should use `RegisterHotKey` from `winuser.h`.

Syntax:
```cpp
BOOL RegisterHotKey(
  [in, optional] HWND hWnd,
  [in]           int  id,
  [in]           UINT fsModifiers,
  [in]           UINT vk
);
```

### Params

* `hWnd`
  * Window that receives `WM_HOTKEY`
  * Can be `NULL` if using global hotkey, so that's what we should do

* `id`
  * Unique identifier for the hotkey
  * Used to distinguish multiple hotkeys
  * We can just make this the index of the action on the wheel (so like 1, 2, 3, etc)

* `fsModifiers`
  * Modifier keys bitmask:
    * `MOD_ALT` (alt key)
    * `MOD_CONTROL` (ctrl key)
    * `MOD_SHIFT` (shift key)
    * `MOD_WIN` (win key)
  * Using any of these means this key would also have to be pressed for the hotkey to work (kinda limited?)

* `vk`
  * Virtual key code (e.g. `VK_SPACE`, `VK_O`, etc)
  * We would have to collect the keypresses from the user and then convert them to virtual key codes

## Receiving the Hotkey

When triggered, Windows sends `WM_HOTKEY` and we will need a message loop to capture it:
```cpp
MSG msg;
while (GetMessage(&msg, NULL, 0, 0)) {
    if (msg.message == WM_HOTKEY) {
        int id = static_cast<int>(msg.wParam);
        // check if the id matches the id of any of our registered hotkeys
        // if it does, open the launcher
    }
    // we need to pass control back to os after checking
    TranslateMessage(&msg);
    DispatchMessage(&msg);
}
```

## Unregister

Unregistering hotkeys is pretty simple:
```cpp
UnregisterHotKey(NULL, id);
```

## Window Focus Info

To bring launcher forward we can use:
* `ShowWindow(hwnd, SW_SHOW)`
* `SetForegroundWindow(hwnd)`
* `SetFocus(hwnd)`

I don't know yet which one is optimial for our use case.
Apparenly Windows blocks focus stealing sometimes, so we may need to use `AttachThreadInput` or `AllowSetForegroundWindow`, but we will see.

## Threading Note

To keep the main thread free, we will probably have to put this loop on a separate thread so it keeps everything else responsive.

## Error Handling

* Hotkey fails if already registered by another app
* Must check return value of `RegisterHotKey` since it returns a boolean
* Some system combos are reserved (Win+L, Ctrl+Alt+Del)
* `GetLastError()` will return the error code if the return is false, most common error for us probably will be `ERROR_HOTKEY_ALREADY_REGISTERED`
* We should probably have a way to gracefully handle these errors
