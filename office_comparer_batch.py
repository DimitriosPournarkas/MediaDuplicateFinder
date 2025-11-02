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
    """Compare Excel files using pandas - flexible comparison"""
    try:
        # Read Excel files
        df1 = pd.read_excel(file1, sheet_name=None)
        df2 = pd.read_excel(file2, sheet_name=None)
        
        # Get common sheet names
        common_sheets = set(df1.keys()).intersection(set(df2.keys()))
        
        if not common_sheets:
            return 0.0
            
        total_similarity = 0
        sheet_count = 0
        
        for sheet_name in common_sheets:
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
    """Compare two Excel sheets - flexible size comparison"""
    # Get dimensions
    rows1, cols1 = df1.shape
    rows2, cols2 = df2.shape
    
    # Find common columns
    common_cols = set(df1.columns).intersection(set(df2.columns))
    if not common_cols:
        return 0.0
    
    # Compare overlapping region only
    min_rows = min(rows1, rows2)
    matching_cells = 0
    compared_cells = 0
    
    for col in common_cols:
        for i in range(min_rows):
            val1 = df1[col].iloc[i]
            val2 = df2[col].iloc[i]
            
            compared_cells += 1
            
            # Handle NaN values
            if pd.isna(val1) and pd.isna(val2):
                matching_cells += 1
            elif str(val1) == str(val2):
                matching_cells += 1
    
    # Calculate similarity based on compared cells
    return matching_cells / compared_cells if compared_cells > 0 else 0.0

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
    """Calculate similarity between two texts - same as word_comparer.py"""
    if not text1 or not text2:
        return 0.0
    
    # Simple word-based similarity (SAME as original word_comparer.py)
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
    """
    results = []
    
    for comp in comparisons:
        file_type = comp['type']
        file1 = comp['file1']
        file2 = comp['file2']
        
        
        # Handle Excel files
        if file_type == 'excel':
            similarity = compare_excel_files(file1, file2)
            similar = similarity > 0.7
            print(f"DEBUG: Excel similarity={similarity}, similar={similar}", file=sys.stderr)
            results.append({'similar': similar, 'score': similarity if similar else 0.0})
        
        # Handle Word files
        elif file_type == 'word':
            text1 = extract_word_text(file1)
            text2 = extract_word_text(file2)
        
            
            if text1 is None or text2 is None:
                results.append({'similar': False, 'score': 0.0})
            else:
                similarity = calculate_text_similarity(text1, text2)
                similar = similarity > 0.6
                results.append({'similar': similar, 'score': similarity if similar else 0.0})
                
        elif file_type == 'powerpoint':
            text1 = extract_powerpoint_text(file1)
            text2 = extract_powerpoint_text(file2)
            
            if text1 is None or text2 is None:
                results.append({'similar': False, 'score': 0.0})
            else:
                similarity = calculate_text_similarity(text1, text2)
                similar = similarity > 0.6
                
                # DEBUG - NEU!
                import os
                f1_name = os.path.basename(file1)
                f2_name = os.path.basename(file2)
                print(f"DEBUG PPT: {f1_name} vs {f2_name} → similarity={similarity:.2f}, similar={similar}", file=sys.stderr)
                
                results.append({'similar': similar, 'score': similarity if similar else 0.0})

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