# OSD Lock Indicator - Complete Uninstall Guide

## ✅ Proper Uninstall Method (Recommended)

The uninstall command now **automatically terminates running processes** for you! No need to manually open Task Manager.

### Step 1: Navigate to the Folder

1. Open File Explorer
2. Navigate to where you saved `OsdLockIndicator.exe`
   - Common locations:
     - `C:\Users\[YourName]\Downloads`
     - `C:\Program Files\OsdLockIndicator`
     - Desktop
     - Custom folder you chose

### Step 2: Open Terminal in That Folder

**Method 1: Right-Click Menu**
1. Right-click on **empty space** in the folder (not on the file!)
2. Select **"Open in Terminal"**

**Method 2: Address Bar Method**
1. Click in the address bar at the top of File Explorer
2. Type: `cmd` or `powershell`
3. Press Enter

**Method 3: Manual Navigation**
1. Press `Win + R`
2. Type: `cmd`
3. Press Enter
4. Type: `cd "path\to\folder"` (replace with actual path)
5. Press Enter

### Step 3: Run the Uninstall Command

In the terminal window, type:

```powershell
.\OsdLockIndicator.exe /uninstall
```

Then press Enter.

**What happens:**
- The program automatically terminates any running instances
- A message box appears showing:
  - ✓ Running processes terminated
  - ✓ Removed from Windows startup
  - "You can now safely delete the executable file."
- Click OK

### Step 4: Delete the File

1. Close the terminal window
2. Delete `OsdLockIndicator.exe` from the folder
3. Empty your Recycle Bin (optional)

**Done!** The program is completely removed.

---

## 🔧 Troubleshooting

### ".\OsdLockIndicator.exe : File cannot be loaded because running scripts is disabled"

**Solution:** You're in PowerShell with restricted execution. Use this instead:
```powershell
& ".\OsdLockIndicator.exe" /uninstall
```

Or simply use Command Prompt instead of PowerShell.

---

### "'.\OsdLockIndicator.exe' is not recognized"

**Problem:** You're not in the right folder.

**Solution:**
1. Type `dir` and press Enter to see files in current folder
2. If you don't see `OsdLockIndicator.exe` listed, you're in the wrong folder
3. Navigate to the correct folder using `cd "path\to\folder"`

---

### "Access is denied" or file won't delete

**Problem:** Another process may have locked the file.

**Solution:**
1. Wait a few seconds after running the uninstall command
2. Try deleting again
3. If still failing, restart your computer and try again

---

### I already deleted the file without uninstalling!

**No problem!** Just clean the registry manually:

1. Press `Win + R`
2. Type: `regedit`
3. Press Enter
4. Navigate to: `HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run`
5. Look for `OsdLockIndicator` in the right panel
6. Right-click it → Delete
7. Close Registry Editor
8. Open Task Manager (`Ctrl+Shift+Esc`) and end any `OsdLockIndicator.exe` processes

---

### Uninstall command shows error about registry

**Problem:** Registry entry couldn't be removed (rare).

**Solution:** Remove it manually:
1. Press `Win + R`
2. Type: `regedit`
3. Press Enter
4. Navigate to: `HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run`
5. Right-click `OsdLockIndicator` → Delete
6. Close Registry Editor

The message confirms the process was still terminated successfully.

---

## ❌ What NOT to Do

### ❌ Don't just delete the file without uninstalling
- Registry entry stays forever
- Windows tries to run it every boot (and fails silently)
- Clutters your registry
- Process may still be running

### ❌ Don't use Task Manager "Disable"
- This doesn't remove the registry entry
- File is still taking up space
- Not a clean removal

---

## 📋 Quick Command Reference

### For PowerShell:
```powershell
.\OsdLockIndicator.exe /uninstall
```

### For Command Prompt:
```cmd
OsdLockIndicator.exe /uninstall
```

### Alternative flags (all work the same):
```
OsdLockIndicator.exe /uninstall
OsdLockIndicator.exe --uninstall
OsdLockIndicator.exe -uninstall
```

---

## ✨ Why This Method is Better

**Traditional Delete:**
- ❌ Registry entry stays
- ❌ Process still running
- ❌ Tries to run every boot
- ❌ Leaves clutter

**Proper Uninstall:**
- ✅ Automatically terminates processes
- ✅ Removes registry entry
- ✅ Clean removal
- ✅ No leftover clutter
- ✅ No manual Task Manager needed

---

## 🎯 Summary

**Best Practice Uninstall (3 Easy Steps):**
1. Open terminal in folder: Right-click on empty space → "Open in Terminal"
2. Run: `.\OsdLockIndicator.exe /uninstall`
   - ✓ Processes automatically terminated
   - ✓ Registry cleaned
3. Delete the file
4. Done!

**If you already deleted:**
- Clean registry manually with `regedit`
- Navigate to: `HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run`
- Delete `OsdLockIndicator` entry
- Open Task Manager and end any `OsdLockIndicator.exe` processes

---

## 💡 New Feature Highlight

**Version 1.1+ includes automatic process termination!**

Previously, you had to:
1. Manually open Task Manager
2. End the process
3. Then run uninstall command

Now, the uninstall command does everything automatically:
- Finds all running instances
- Terminates them safely
- Removes startup entry
- Ready to delete immediately

This makes uninstallation faster and easier for everyone!

---

**Questions?** Check the main README FAQ section or open an issue on GitHub.
