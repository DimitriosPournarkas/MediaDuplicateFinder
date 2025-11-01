import sys
import json
from docx import Document
import pandas as pd
from pptx import Presentation
import re

def extract_word_text(filepath):
    """Extract text from Word document"""
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
    except Exception as e:
        return None

def compare_excel_files(file1, file2):
    """Compare Excel files using pandas (same logic as excel_comparer.py)"""
    try:
        # Read Excel files
        df1 = pd.read_excel(file1, sheet_name=None)
        df2 = pd.read_excel(file2, sheet_name=None)
        
        # Check if same number of sheets
        if len(df1) != len(df2):
            return 0.0
            
        total_similarity = 0
        sheet_count = 0
        
        for sheet_name in df1.keys():
            if sheet_name in df2:
                sheet_similarity = compare_sheets(df1[sheet_name], df2[sheet_name])
                total_similarity += sheet_similarity
                sheet_count += 1
        
        if sheet_count == 0:
            return 0.0
            
        overall_similarity = total_similarity / sheet_count
        return overall_similarity
        
    except Exception as e:
        return 0.0

def compare_sheets(df1, df2):
    """Compare two Excel sheets"""
    # Compare basic structure
    if df1.shape != df2.shape:
        return 0.0
    
    # Compare data types
    if len(df1.dtypes) != len(df2.dtypes):
        return 0.0
    
    # Simple content comparison
    matching_cells = 0
    total_cells = df1.size
    
    for col in df1.columns:
        if col in df2.columns:
            for i in range(min(len(df1), len(df2))):
                val1 = df1[col].iloc[i] if i < len(df1) else None
                val2 = df2[col].iloc[i] if i < len(df2) else None
                
                # Handle NaN values
                if pd.isna(val1) and pd.isna(val2):
                    matching_cells += 1
                elif str(val1) == str(val2):
                    matching_cells += 1
    
    return matching_cells / total_cells if total_cells > 0 else 0.0

def extract_powerpoint_text(filepath):
    """Extract text from PowerPoint"""
    try:
        prs = Presentation(filepath)
        text = ""
        for slide in prs.slides:
            for shape in slide.shapes:
                if hasattr(shape, "text"):
                    text += shape.text + " "
        return text.strip()
    except Exception as e:
        return None

def calculate_text_similarity(text1, text2):
    """Calculate similarity between two texts"""
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
    """
    Compare multiple file pairs at once
    Input: List of {type, file1, file2}
    Output: List of similarity scores
    """
    results = []
    
    for comp in comparisons:
        file_type = comp['type']
        file1 = comp['file1']
        file2 = comp['file2']
        
        # Extract text based on type
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
        
        # Calculate similarity
        if text1 is None or text2 is None:
            results.append({'similar': False, 'score': 0.0})
        else:
            similarity = calculate_text_similarity(text1, text2)
            similar = similarity > 0.6
            results.append({'similar': similar, 'score': similarity if similar else 0.0})
    
    return results

if __name__ == "__main__":
    # Read JSON from stdin
    input_data = sys.stdin.read()
    
    try:
        comparisons = json.loads(input_data)
        results = compare_files_batch(comparisons)
        
        # Output results as JSON
        print(json.dumps(results))
        sys.exit(0)
    except Exception as e:
        print(json.dumps({'error': str(e)}), file=sys.stderr)
        sys.exit(1)