# Handoff Report — Challenger 2 Stress/Boundary Tests

## Challenge Summary

**Overall risk assessment**: **HIGH** (The desktop slideshow feature is completely non-functional due to a critical race condition).

---

## 1. Observations

### Verbatim logs and commands:
- Running the stress test script `Subagent_Tests\run_challenger_tests.ps1`:
  ```
  [TEST 1B] Testing behavior with exactly 3 seconds interval...
  Monitoring wallpaper switches (should occur every 3 seconds)...
    [DEBUG] Iteration 0 : img1 locked=False, img2 locked=False, currentLocked=
    ...
    [DEBUG] Iteration 47 : img1 locked=False, img2 locked=False, currentLocked=
  [FAIL] Slideshow did not cycle within the test duration. Make sure files are present and slideshow runs.
  ```
- File lock status remains `False` for both `img1.jpg` and `img2.jpg` throughout the duration of the test.
- The same behavior is observed in Test 1C (Large Number Overflow).
- Test 1A (Missing Registry Keys), Test 1D (Invalid Characters clamped to 3s), Test 2 (WM_DISPLAYCHANGE handling), and Test 3 (Settings CPL sync/temp file cleanup) all reported **PASS**.

---

## 2. Logic Chain

1. **Gdiplus Locking Behavior**: Empirical verification using `Subagent_Tests\test_lock.ps1` confirmed that when a `Gdiplus::Bitmap` object is loaded, it locks the file on disk, which is detected as `True` by `Is-FileLocked`. When deleted, the lock is released (verified by `Subagent_Tests\test_gdiplus_lock.cpp`).
2. **Slideshow Timer Failure**: During the first paint message (`WM_PAINT` or `WM_ERASEBKGND`) dispatched during `CreateWindowExW` execution in `DesktopWindow::Initialize()`, `s_hProgman` has not yet been assigned the return value of `CreateWindowExW` and is thus `NULL`.
3. **NULL Timer Creation**: The function `DrawWallpaper` attempts to set the slideshow timer using:
   ```cpp
   SetTimer(s_hProgman, TIMER_SLIDESHOW, slideshowInterval * 1000, NULL);
   ```
   Since `s_hProgman` is `NULL`, `SetTimer` creates a thread-level timer rather than a window-associated timer.
4. **Mismatched Cache State**: The static cache variables `s_lastSlideshowEnabled` and `s_lastSlideshowInterval` are updated to `1` and `3` during this first paint. On subsequent repaints, since the registry values match the cached values, `SetTimer` is never invoked again (even though `s_hProgman` has since become non-NULL).
5. **No Slideshow Cycle**: As a result, the slideshow timer is never associated with the custom Progman window. `ProgmanWndProc` never receives `WM_TIMER` messages, `AdvanceSlideshow` is never called, and no wallpaper files are ever loaded or locked.

---

## 3. Caveats

- We assumed that there is only one custom Progman window, which is true since it spans the virtual screen (`SM_XVIRTUALSCREEN`).
- We verified that the test files `img1.jpg` and `img2.jpg` exist on disk and GDI+ can successfully open them under ordinary conditions (verified by `test_slideshow_diag.exe`).

---

## 4. Conclusion

- **FINAL VERDICT**: **FAIL**
- While display configuration changes, missing registry keys, and settings synchronization/CPL applet cleanup pass all criteria, the desktop slideshow feature **fails** due to a critical initialization race condition in `DesktopWindow.cpp`.

---

## 5. Verification Method

To verify the race condition:
1. Run the test script `Subagent_Tests\run_challenger_tests.ps1`.
2. Inspect the output of the monitoring loops for Test 1B and Test 1C to see that neither file is ever locked (`locked=False` for both).
3. Check `Subagent_Tests\challenger_verdict.txt` to confirm the final result.

---

## Stress Test Results

- **Test 1A (Missing Registry Keys)** → Start `EliteTaskbar.exe` without keys → Process runs stably → **PASS**
- **Test 1B (Exact 3s Interval)** → Monitor locks → Neither file locked → **FAIL**
- **Test 1C (Large Number Overflow)** → Set interval to 4294968s → Neither file locked → **FAIL**
- **Test 1D (Invalid Characters in UI)** → Send "abc" to interval edit → Registry clamped to 3s → **PASS**
- **Test 2 (WM_DISPLAYCHANGE)** → Send WM_DISPLAYCHANGE message → Process does not crash → **PASS**
- **Test 3 (CPL Applet Sync)** → Launch CPL, close dialog → Temp file deleted successfully → **PASS**

## Adversarial Findings

### 1. Critical Initialization Timer Race Condition
- **Assumption Challenged**: That the static variable `s_hProgman` is initialized and valid when `DrawWallpaper` is called.
- **Attack Scenario**: Setting the window style `WS_VISIBLE` forces a paint event during window construction inside `CreateWindowExW`, before the handle can be assigned to `s_hProgman`.
- **Blast Radius**: Slideshow timer is never registered on the window, breaking slideshow functionality entirely.
- **Mitigation**: Update `DrawWallpaper` to accept the window handle (`hwnd`) directly as a parameter, or check if `s_hProgman` is `NULL` and delay setting the static state variables until a valid handle is present. Or obtain the window handle using `WindowFromDC(hdc)`.

### 2. Test Script Is-FileLocked False Positives
- **Assumption Challenged**: That `Is-FileLocked` only returns `True` if the file is locked by a process.
- **Attack Scenario**: If the target file is missing/deleted, `[System.IO.File]::Open` throws a `FileNotFoundException`, triggering the `catch` block and incorrectly returning `True`.
- **Blast Radius**: Distorts diagnostic results when files are not present.
- **Mitigation**: Add `if (!(Test-Path $path)) { return $false }` to the beginning of `Is-FileLocked` to handle non-existent files cleanly.
