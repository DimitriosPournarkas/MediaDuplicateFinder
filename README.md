# MediaDuplicateFinder

MediaDuplicateFinder is a high-performance duplicate file finder that supports **any type of file** — including images, audio, text, Office documents, and more. It can detect **exact duplicates** as well as **similar files**, even across different formats, sizes, and quality levels.

The main program is implemented in **C++** for maximum speed and efficiency. For comparing Office documents, the Python script `Office_comparer_batch.py` is used. A **user-friendly GUI** built in Python provides easy access to all features, including scanning, filtering, and deleting duplicates.

## Key Features
- Fast duplicate detection for large file collections
- Detection of similar files using perceptual hashing (images) and audio fingerprinting (audio)
- Support for text and Office files
- Interactive Python GUI for browsing, filtering, and deleting duplicates
- Cross-platform compatibility (Windows, Linux)

## Usage
1. Run the main executable (`duplicate_finder.exe` on Windows or `duplicate_finder` on Linux) to scan files.
2. Use the Python GUI to browse results, filter by filename or directory, and delete duplicates safely.
3. For Office documents, run `Office_comparer_batch.py` to compare and find duplicates.

## Installation
- Ensure Python 3.x is installed.
- Install required packages: `pip install -r requirements.txt`
- Make sure the C++ executable is built and available in the repository folder.

## License
[MIT License](LICENSE)

