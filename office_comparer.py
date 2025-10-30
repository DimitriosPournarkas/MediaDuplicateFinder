# office_comparer.py
import sys, os
from excel_comparer import compare_excel
from word_comparer import compare_word
from powerpoint_comparer import compare_powerpoint

def main():
    if len(sys.argv) < 3:
        print("Usage: office_comparer.py file1 file2")
        return

    file1, file2 = sys.argv[1], sys.argv[2]
    ext1 = os.path.splitext(file1)[1].lower()
    ext2 = os.path.splitext(file2)[1].lower()

    # Excel/CSV
    if {ext1, ext2} & {".xls", ".xlsx", ".csv"}:
        compare_excel(file1, file2)
    # Word
    elif {ext1, ext2} & {".doc", ".docx"}:
        compare_word(file1, file2)
    # PowerPoint
    elif {ext1, ext2} & {".ppt", ".pptx"}:
        compare_powerpoint(file1, file2)
    else:
        print(f"Unsupported file types: {ext1}, {ext2}")

if __name__ == "__main__":
    main()
