## Secure Process Launching

For running other exe's securely on Windows, we should avoid `system()` at all costs. It's incredibly unsafe because it passes strings directly to the command interpreter (`cmd.exe`), opening us up to command injection.

We should use `CreateProcessW` from `processthreadsapi.h`.

Syntax:
```cpp
BOOL CreateProcessW(
  [in, optional]      LPCWSTR               lpApplicationName,
  [in, out, optional] LPWSTR                lpCommandLine,
  [in, optional]      LPSECURITY_ATTRIBUTES lpProcessAttributes,
  [in, optional]      LPSECURITY_ATTRIBUTES lpThreadAttributes,
  [in]                BOOL                  bInheritHandles,
  [in]                DWORD                 dwCreationFlags,
  [in, optional]      LPVOID                lpEnvironment,
  [in, optional]      LPCWSTR               lpCurrentDirectory,
  [in]                LPSTARTUPINFOW        lpStartupInfo,
  [out]               LPPROCESS_INFORMATION lpProcessInformation
);

```

### Params & Security Considerations

* `lpApplicationName` vs `lpCommandLine`
* This is a massive security issue. If we pass `NULL` to `lpApplicationName` and pass a spaces-containing path in `lpCommandLine` (like `C:\Program Files\App\test.exe`), Windows might try to execute `C:\Program.exe` first if it's not quoted properly.
* Always pass the absolute path directly to `lpApplicationName`, OR wrap the executable path in double quotes inside `lpCommandLine`.

* `bInheritHandles`
* Set this to `FALSE` unless we explicitly need the child process to inherit open files, sockets, or pipes from our parent process. Leaving this `TRUE` by default is a privilege leak risk.

* `dwCreationFlags`
* We can use `CREATE_NO_WINDOW` if we are running background scripts/tools and don't want a random console window popping up and interrupting the user.

## Running Scripts Safely (Powershell / Batch)

We shouldn't launch scripts directly. We have to spin up the interpreter (`powershell.exe` or `cmd.exe`) and pass the script path as an argument.

### Powershell Best Practices

* By default, Windows blocks script execution. We have to bypass it, but we should do it strictly for our process scope, not system-wide.
* If the script path or arguments come from user input, we have to sanitize them to prevent injection.

For example:
```cpp
// point to the system powershell executable to prevent dll hijacking
std::wstring psPath = L"C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe";

// Use -NoProfile and -ExecutionPolicy Bypass for a clean, isolated run
std::wstring args = L" -NoProfile -ExecutionPolicy Bypass -File \"C:\\path\\to\\our\\script.ps1\"";

STARTUPINFOW si = { sizeof(si) };
PROCESS_INFORMATION pi;

// lpCommandLine must be a modifiable string, so copy it to a buffer
std::vector<wchar_t> cmdBuffer(args.begin(), args.end());
cmdBuffer.push_back(L'\0');

if (CreateProcessW(psPath.c_str(), cmdBuffer.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
    // wait for it to finish if we need to block, otherwise close handles immediately
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}
```

## Handling Process Outputs

* We have to use anonymous pipes. We can use `CreatePipe` to get read/write handles.
* Pass the write handle to the child's `hStdOutput` and `hStdError` inside `STARTUPINFOW` (which requires setting the `STARTF_USESTDHANDLES` flag).
* Ensure only the write handle is inherited (`SetHandleInformation` with `HANDLE_FLAG_INHERIT`), otherwise the child process might hang waiting for the pipe to close.

## Sandbox / Integrity Levels

By default, the launched process runs with the same privileges as our app.

* We could look into token restriction APIs (`CreateProcessAsUserW` or using AppContainer isolation) to drop privileges to a low integrity level, but that's probably overkill since the user is explicitly trusting us.

## Error Handling & Cleanup

* Always check the return value of `CreateProcessW` (returns zero on failure).
* Use `GetLastError()` to diagnose. Common issues:
    * `ERROR_FILE_NOT_FOUND` (typo in path or missing interpreter)
    * `ERROR_ACCESS_DENIED` (permissions issue or antivirus blocking)


We have to call `CloseHandle` on both `pi.hProcess` and `pi.hThread` as soon as we are done with them, even if the process is still running. Apparently Windows keeps the process object alive in memory until all handles are closed.