import pandas as pd
import sys
import os

def compare_excel_files(file1, file2):
    try:
        
        
        # Read Excel files
        df1 = pd.read_excel(file1, sheet_name=None)
        df2 = pd.read_excel(file2, sheet_name=None)
        
        # Check if same number of sheets
        if len(df1) != len(df2):
            return False
            
        total_similarity = 0
        sheet_count = 0
        
        for sheet_name in df1.keys():
            if sheet_name in df2:
                sheet_similarity = compare_sheets(df1[sheet_name], df2[sheet_name])
                total_similarity += sheet_similarity
                sheet_count += 1
               
        
        if sheet_count == 0:
            return False
            
        overall_similarity = total_similarity / sheet_count
       
        
        return overall_similarity > 0.7  # 70% similarity threshold
        
    except Exception as e:
        
        return False

def compare_sheets(df1, df2):
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

if __name__ == "__main__":
    if len(sys.argv) != 3:
        
        sys.exit(1)
    
    file1, file2 = sys.argv[1], sys.argv[2]
    similar = compare_excel_files(file1, file2)
    sys.exit(0 if similar else 1)