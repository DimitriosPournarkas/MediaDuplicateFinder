import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import subprocess
import os
from pathlib import Path

class DuplicateFinderGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Duplicate File Finder - C++ Backend")
        self.root.geometry("900x600")
    
    # C++ executable path - AUTOMATISCHE .exe ERKENNUNG

        current_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Prüfe ob wir auf Windows sind und füge .exe hinzu
        if os.name == 'nt':
            self.cpp_executable = os.path.join(current_dir, "duplicate_finder.exe")
        else:
            self.cpp_executable = os.path.join(current_dir, "duplicate_finder")
    
        print(f"C++ executable path: {self.cpp_executable}")  # Debug output
    
        self.setup_ui()
        
    def setup_ui(self):
        # Main frame
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # Directory selection
        dir_frame = ttk.Frame(main_frame)
        dir_frame.pack(fill=tk.X, pady=5)
        
        ttk.Label(dir_frame, text="Select Directory:").pack(side=tk.LEFT)
        
        self.dir_var = tk.StringVar()
        self.dir_entry = ttk.Entry(dir_frame, textvariable=self.dir_var, width=60)
        self.dir_entry.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)
        
        ttk.Button(dir_frame, text="Browse", command=self.browse_directory).pack(side=tk.LEFT)
        
        # Buttons
        button_frame = ttk.Frame(main_frame)
        button_frame.pack(fill=tk.X, pady=10)
        
        ttk.Button(button_frame, text="Scan Duplicates", 
                  command=self.scan_duplicates).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Delete All Duplicates", 
                  command=self.delete_all_duplicates).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Clear Results", 
                  command=self.clear_results).pack(side=tk.LEFT, padx=5)
        
        # Results
        results_frame = ttk.LabelFrame(main_frame, text="Results", padding="5")
        results_frame.pack(fill=tk.BOTH, expand=True, pady=10)
        
        # Treeview for results
        self.tree = ttk.Treeview(results_frame, columns=("size",), show="tree headings", height=15)
        self.tree.pack(fill=tk.BOTH, expand=True)
        
        # Scrollbar
        scrollbar = ttk.Scrollbar(results_frame, orient=tk.VERTICAL, command=self.tree.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.tree.configure(yscrollcommand=scrollbar.set)
        
        # Configure columns
        self.tree.heading("#0", text="Duplicate Files")
        self.tree.column("#0", width=600)
        self.tree.heading("size", text="Size")
        self.tree.column("size", width=100)
        
        # Status
        self.status_var = tk.StringVar(value="Ready - Select directory and click Scan")
        status_bar = ttk.Label(main_frame, textvariable=self.status_var, relief=tk.SUNKEN)
        status_bar.pack(fill=tk.X, pady=5)
        
        self.duplicate_groups = []
        
    def browse_directory(self):
        directory = filedialog.askdirectory(title="Select Directory to Scan")
        if directory:
            self.dir_var.set(directory)
            self.status_var.set(f"Selected directory: {directory}")
    
    def scan_duplicates(self):
        directory = self.dir_var.get()
        if not directory or not os.path.exists(directory):
            messagebox.showerror("Error", "Please select a valid directory")
            return
        
        if not os.path.exists(self.cpp_executable):
            messagebox.showerror("Error", "C++ backend not found. Please compile first.")
            return
        
        self.clear_results()
        self.status_var.set("Scanning directory with C++ backend...")
        self.root.update()
        
        try:
            # C++ Programm aufrufen
            result = subprocess.run(
                [self.cpp_executable, directory],
                capture_output=True,
                text=True,
                timeout=300  # 5 minutes timeout
            )
            
            if result.returncode != 0:
                messagebox.showerror("Error", f"C++ backend error:\n{result.stderr}")
                return
            
            # Ergebnisse parsen
            self.parse_results(result.stdout)
            
        except subprocess.TimeoutExpired:
            messagebox.showerror("Error", "Scanning timed out after 5 minutes")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to run C++ backend: {e}")
        
        self.status_var.set("Ready")
    
    def parse_results(self, output):
        groups = []
        current_group = []
        
        for line in output.strip().split('\n'):
            if line == "---GROUP---":
                if current_group:
                    groups.append(current_group)
                    current_group = []
            else:
                current_group.append(line)
        
        if current_group:
            groups.append(current_group)
        
        self.duplicate_groups = groups
        self.display_results()
    
    def display_results(self):
        if not self.duplicate_groups:
            messagebox.showinfo("No Duplicates", "No duplicate files found!")
            return
        
        group_count = 0
        for group in self.duplicate_groups:
            if len(group) > 1:
                group_count += 1
                group_item = self.tree.insert("", "end", text=f"Duplicate Group {group_count} ({len(group)} files)")
                
                for file_path in group:
                    file_size = os.path.getsize(file_path) if os.path.exists(file_path) else 0
                    size_str = f"{file_size / 1024:.1f} KB" if file_size > 0 else "N/A"
                    
                    file_item = self.tree.insert(group_item, "end", text=os.path.basename(file_path), 
                                               values=(size_str,))
                    self.tree.set(file_item, "size", size_str)
                
                self.tree.item(group_item, open=True)
        
        self.status_var.set(f"Found {group_count} duplicate groups")
    
    def delete_all_duplicates(self):
        if not self.duplicate_groups:
            messagebox.showinfo("Info", "No duplicates to delete")
            return
        
        # Count files to delete
        total_to_delete = 0
        for group in self.duplicate_groups:
            total_to_delete += len(group) - 1  # Keep first file
        
        if total_to_delete == 0:
            messagebox.showinfo("Info", "No duplicates to delete")
            return
        
        # Confirmation
        confirm = messagebox.askyesno(
            "Confirm Deletion",
            f"This will delete {total_to_delete} duplicate files while keeping 1 original for each group.\n\nAre you sure?"
        )
        
        if not confirm:
            return
        
        # Delete files
        deleted_count = 0
        errors = []
        
        for group in self.duplicate_groups:
            if len(group) > 1:
                # Keep first file, delete the rest
                for file_path in group[1:]:
                    try:
                        os.remove(file_path)
                        deleted_count += 1
                    except Exception as e:
                        errors.append(f"{file_path}: {e}")
        
        # Show results
        if errors:
            error_msg = f"Deleted {deleted_count} files, but encountered errors:\n\n" + "\n".join(errors[:5])
            if len(errors) > 5:
                error_msg += f"\n\n... and {len(errors) - 5} more errors"
            messagebox.showerror("Deletion Complete with Errors", error_msg)
        else:
            messagebox.showinfo("Deletion Complete", f"Successfully deleted {deleted_count} duplicate files")
        
        self.status_var.set(f"Deleted {deleted_count} files")
        self.scan_duplicates()  # Refresh results
    
    def clear_results(self):
        for item in self.tree.get_children():
            self.tree.delete(item)
        self.duplicate_groups = []
        self.status_var.set("Results cleared")

if __name__ == "__main__":
    root = tk.Tk()
    app = DuplicateFinderGUI(root)
    root.mainloop()