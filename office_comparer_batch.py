import sys
import json
from docx import Document
from openpyxl import load_workbook
from pptx import Presentation
import re

def extract_word_text(filepath):
    try:
        doc = Document(filepath)
        text = ""
        for paragraph in doc.paragraphs:
            text += paragraph.text + "\n"
        for table in doc.tables:
            for row in table.rows:
                for cell in row.cells:
                    text += cell.text + " "
        return text.strip()
    except:
        return None

def extract_excel_text(filepath):
    try:
        wb = load_workbook(filepath, read_only=True, data_only=True)
        text = ""
        for sheet in wb.worksheets:
            for row in sheet.iter_rows(values_only=True):
                for cell in row:
                    if cell is not None:
                        text += str(cell) + " "
        wb.close()
        return text.strip()
    except:
        return None

def extract_powerpoint_text(filepath):
    try:
        prs = Presentation(filepath)
        text = ""
        for slide in prs.slides:
            for shape in slide.shapes:
                if hasattr(shape, "text"):
                    text += shape.text + " "
        return text.strip()
    except:
        return None

def calculate_text_similarity(text1, text2):
    if not text1 or not text2:
        return 0.0
    
    words1 = set(re.findall(r'\w+', text1.lower()))
    words2 = set(re.findall(r'\w+', text2.lower()))
    
    if not words1 or not words2:
        return 0.0
    
    common_words = words1.intersection(words2)
    similarity = len(common_words) / max(len(words1), len(words2))
    
    return similarity

def compare_files_batch(comparisons):
    results = []
    
    for comp in comparisons:
        file_type = comp['type']
        file1 = comp['file1']
        file2 = comp['file2']
        
        if file_type == 'word':
            text1 = extract_word_text(file1)
            text2 = extract_word_text(file2)
        elif file_type == 'excel':
            text1 = extract_excel_text(file1)
            text2 = extract_excel_text(file2)
        elif file_type == 'powerpoint':
            text1 = extract_powerpoint_text(file1)
            text2 = extract_powerpoint_text(file2)
        else:
            results.append({'similar': False, 'score': 0.0})
            continue
        
        if text1 is None or text2 is None:
            results.append({'similar': False, 'score': 0.0})
        else:
            similarity = calculate_text_similarity(text1, text2)
            similar = similarity > 0.6
            results.append({'similar': similar, 'score': similarity if similar else 0.0})
    
    return results

if __name__ == "__main__":
    input_data = sys.stdin.read()
    
    try:
        comparisons = json.loads(input_data)
        results = compare_files_batch(comparisons)
        print(json.dumps(results))
        sys.exit(0)
    except Exception as e:
        print(json.dumps({'error': str(e)}), file=sys.stderr)
        sys.exit(1)