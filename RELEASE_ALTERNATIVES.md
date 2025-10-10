# Alternative Solutions for Large Release Package

The complete package (~27 MB) exceeds GitHub's 25 MB limit for release assets. Here are several solutions:

## Solution 1: Split into Multiple Files (RECOMMENDED)

Split the package into two parts:

### Part 1: Core Application (~15 MB)
- Executable and essential Qt DLLs
- Platform plugins

### Part 2: ICU Data (~12 MB)
- icudt74.dll (internationalization data)

Users download both and extract to the same folder.

## Solution 2: Use External Hosting

Host the package on:

### Google Drive
1. Upload to Google Drive
2. Set sharing to "Anyone with the link"
3. Get shareable link
4. Update README with the link

### OneDrive / Dropbox
Similar process - upload and share link

### Release.md / SourceForge
Free hosting for open source projects

## Solution 3: Use Git LFS (GitHub Large File Storage)

GitHub LFS allows files up to 2 GB, but requires:
- Git LFS extension installed
- Storage quota (1 GB free, then paid)

## Solution 4: Compress Executable with UPX

Use UPX (Ultimate Packer for eXecutables) to compress:
- Can reduce .exe size by 50-70%
- May trigger antivirus false positives
- Download from: https://upx.github.io/

## Solution 5: Host on GitHub Pages

Create a releases page and link to external download

---

## CURRENT RECOMMENDATION: Split Package Method

I'll create two packages:
1. **HeatXTwin_Core.zip** (~15 MB) - Main files
2. **HeatXTwin_Data.zip** (~12 MB) - ICU data file

Both can be uploaded to GitHub Releases separately.

Run: `package_release_split.bat` to create split packages.
