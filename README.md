# Qt Installation & Setup Guide (Windows + Visual Studio)

## Overview

This guide explains how to install **Qt 6** and set it up with **Visual Studio 2026** to build and run the project.

---

## 1. Install Qt

1. Go to the official Qt website:
   https://www.qt.io/download

2. Download the **Qt Online Installer**

3. Run the installer and log in (create a free Qt account if needed)

4. When selecting components, install:

   * **Qt 6.x (latest)**
   * **MSVC 2022 64-bit**
   * Qt Widgets module (included by default)

Example path after install:

```
C:\Qt\6.x.x\msvc2022_64\
```

---

## 2. Install Visual Studio Requirements

Make sure you have **Visual Studio 2026** with:

* Desktop development with C++
* MSVC v143 toolset
* Windows 10/11 SDK

---

## 3. Install Qt Visual Studio Tools

1. Open Visual Studio
2. Go to **Extensions → Manage Extensions**
3. Search for:

   ```
   Qt Visual Studio Tools
   ```
4. Install and restart Visual Studio

---

## 4. Configure Qt in Visual Studio

1. Go to:

   ```
   Extensions → Qt VS Tools → Qt Versions
   ```

2. Click **Add**

3. Select your Qt installation folder:

   ```
   C:\Qt\6.x.x\msvc2026_64\
   ```

4. Apply and close

---

## 5. Open and Build the Project

1. Open the `.sln` file in Visual Studio

2. Set the project as **Startup Project**

3. Select configuration:

   ```
   x64 | Debug
   ```

4. Build:

   ```
   Build → Build Solution
   ```

---

## 6. Run the Application

Click:

```
Local Windows Debugger
```

Expected result:

* Qt UI window appears
* Console window may appear if project is set to Console subsystem

---

## 7. Common Issues

### UI does not show

* Ensure `QApplication` is used (not `QCoreApplication`)
* Ensure `window.show()` is called in `main.cpp`

### Qt DLL errors

* Verify Qt version matches MSVC version (MSVC 2026)
* Rebuild solution

### Plugin error (qwindows.dll)

* Make sure Qt path is correctly configured in Qt VS Tools

---

## 8. Notes

* This project uses **Qt Widgets (not QML)**
* Designed for **Windows + Visual Studio 2026**
* Tested with Qt 6.x (MSVC 64-bit)

---

## Author Notes

If the UI does not appear but the program runs:

* Check threading logic
* Ensure no blocking code runs before `app.exec()`

---
# Qt Installation & Setup Guide (Windows + Visual Studio)

## Overview

This guide explains how to install **Qt 6** and set it up with **Visual Studio 2026** to build and run the project.

---

## 1. Install Qt

1. Go to the official Qt website:
   https://www.qt.io/download

2. Download the **Qt Online Installer**

3. Run the installer and log in (create a free Qt account if needed)

4. When selecting components, install:

   * **Qt 6.x (latest)**
   * **MSVC 2022 64-bit**
   * Qt Widgets module (included by default)

Example path after install:

```
C:\Qt\6.x.x\msvc2022_64\
```

---

## 2. Install Visual Studio Requirements

Make sure you have **Visual Studio 2026** with:

* Desktop development with C++
* MSVC v143 toolset
* Windows 10/11 SDK

---

## 3. Install Qt Visual Studio Tools

1. Open Visual Studio
2. Go to **Extensions → Manage Extensions**
3. Search for:

   ```
   Qt Visual Studio Tools
   ```
4. Install and restart Visual Studio

---

## 4. Configure Qt in Visual Studio

1. Go to:

   ```
   Extensions → Qt VS Tools → Qt Versions
   ```

2. Click **Add**

3. Select your Qt installation folder:

   ```
   C:\Qt\6.x.x\msvc2026_64\
   ```

4. Apply and close

---

## 5. Open and Build the Project

1. Open the `.sln` file in Visual Studio

2. Set the project as **Startup Project**

3. Select configuration:

   ```
   x64 | Debug
   ```

4. Build:

   ```
   Build → Build Solution
   ```

---

## 6. Run the Application

Click:

```
Local Windows Debugger
```

Expected result:

* Qt UI window appears
* Console window may appear if project is set to Console subsystem

---

## 7. Common Issues

### UI does not show

* Ensure `QApplication` is used (not `QCoreApplication`)
* Ensure `window.show()` is called in `main.cpp`

### Qt DLL errors

* Verify Qt version matches MSVC version (MSVC 2026)
* Rebuild solution

### Plugin error (qwindows.dll)

* Make sure Qt path is correctly configured in Qt VS Tools

---

## 8. Notes

* This project uses **Qt Widgets (not QML)**
* Designed for **Windows + Visual Studio 2026**
* Tested with Qt 6.x (MSVC 64-bit)

---

## Author Notes

If the UI does not appear but the program runs:

* Check threading logic
* Ensure no blocking code runs before `app.exec()`

---
