# Creating a GitHub Release

Follow these steps to upload the built executable to GitHub Releases:

## Steps to Create a Release

### 1. Go to GitHub Releases Page
Visit: https://github.com/debdutta777/HeatXTwin_Final/releases

### 2. Click "Create a new release"
Or directly go to: https://github.com/debdutta777/HeatXTwin_Final/releases/new

### 3. Fill in Release Information

**Tag version:** `v1.0.0`
- Click "Choose a tag" and type `v1.0.0`
- Select "Create new tag: v1.0.0 on publish"

**Release title:** `HeatXTwin Pro v1.0.0 - Initial Release`

**Description:** Copy and paste the following:

```markdown
## 🎉 First Release of HeatXTwin Pro!

A complete Heat Exchanger Digital Twin simulation tool with modern Qt6 interface.

### ✨ Features
- ✅ Real-time heat exchanger simulation
- ✅ Interactive charts with hover tooltips
- ✅ Speed control (1×-100× multipliers)
- ✅ Color-coded controls (Green/Yellow/Red buttons)
- ✅ Live parameter adjustment
- ✅ Professional tabbed interface

### 📥 Installation (IMPORTANT: Download BOTH files)

**Step 1:** Download both files below:
- `HeatXTwin_v1.0.0_Core.zip` (~15 MB)
- `HeatXTwin_v1.0.0_Data.zip` (~12 MB)

**Step 2:** Extract BOTH files to the SAME folder

**Step 3:** Run `HeatXTwin_Pro.exe`

⚠️ **Note:** Both files are required. The application will not run without the data file.

### 📋 System Requirements
- Windows 10/11 (64-bit)
- 4 GB RAM (8 GB recommended)
- ~500 MB disk space

### 📚 Documentation
See the repository README and documentation files for detailed usage instructions.

### 🐛 Known Issues
None reported yet. Please open an issue if you encounter any problems.

### 🙏 Credits
Developed as a Major Project - Heat Exchanger Digital Twin Simulation
```

### 4. Upload the Release Packages

Click "Attach binaries by dropping them here or selecting them"

Upload BOTH files:
1. `releases\HeatXTwin_v1.0.0_Core.zip` (15 MB)
2. `releases\HeatXTwin_v1.0.0_Data.zip` (12 MB)

### 5. Options

- ✅ Check "Set as the latest release"
- ⬜ Leave "Set as a pre-release" unchecked (unless testing)

### 6. Publish

Click the green **"Publish release"** button

---

## After Publishing

The release will be available at:
- Direct link: https://github.com/debdutta777/HeatXTwin_Final/releases/tag/v1.0.0
- Latest: https://github.com/debdutta777/HeatXTwin_Final/releases/latest

Users can now download the executable directly from the main README link!

## Updating for Future Releases

1. Modify version in `package_release.bat`
2. Run `package_release.bat` to create new package
3. Create new release with new tag (e.g., `v1.1.0`)
4. Upload the new ZIP file
5. Update README download link if needed

---

**Current Package Locations:**
- Core: `C:\Users\DEBDUTTA\Desktop\MAJOR PROJECT\HeatXTwin_Final\releases\HeatXTwin_v1.0.0_Core.zip`
- Data: `C:\Users\DEBDUTTA\Desktop\MAJOR PROJECT\HeatXTwin_Final\releases\HeatXTwin_v1.0.0_Data.zip`

**Package Sizes:** 
- Core: ~15 MB
- Data: ~12 MB
- Total: ~27 MB (split into 2 files to meet GitHub's 25 MB limit)
