import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import subprocess
import os
import threading
import time

class DuplicateFinderGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Duplicate File Finder - C++ Backend")
        self.root.geometry("1100x650")
    
        current_dir = os.path.dirname(os.path.abspath(__file__))
    
        if os.name == 'nt':
            self.cpp_executable = os.path.join(current_dir, "duplicate_finder.exe")
        else:
            self.cpp_executable = os.path.join(current_dir, "duplicate_finder")
    
        print(f"C++ executable path: {self.cpp_executable}")
    
        self.scanning = False
        self.setup_ui()
        
    def setup_ui(self):
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        dir_frame = ttk.Frame(main_frame)
        dir_frame.pack(fill=tk.X, pady=5)
        
        ttk.Label(dir_frame, text="Select Directory:").pack(side=tk.LEFT)
        
        self.dir_var = tk.StringVar()
        self.dir_entry = ttk.Entry(dir_frame, textvariable=self.dir_var, width=60)
        self.dir_entry.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)
        
        ttk.Button(dir_frame, text="Browse", command=self.browse_directory).pack(side=tk.LEFT)
        
        button_frame = ttk.Frame(main_frame)
        button_frame.pack(fill=tk.X, pady=10)
        
        ttk.Button(button_frame, text="Scan Duplicates & Similar Files", 
                  command=self.scan_duplicates, width=30).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Delete All Exact Duplicates", 
                  command=self.delete_all_duplicates, width=25).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Clear Results", 
                  command=self.clear_results).pack(side=tk.LEFT, padx=5)
        
        # Progress bar
        self.progress_frame = ttk.Frame(main_frame)
        self.progress_frame.pack(fill=tk.X, pady=5)
        
        self.progress_bar = ttk.Progressbar(self.progress_frame, mode='indeterminate')
        self.progress_bar.pack(fill=tk.X, pady=5)
        self.progress_frame.pack_forget()  # Hide initially
        
        results_frame = ttk.LabelFrame(main_frame, text="Results", padding="5")
        results_frame.pack(fill=tk.BOTH, expand=True, pady=10)
        
        self.tree = ttk.Treeview(results_frame, columns=("size", "similarity"), 
                                show="tree headings", height=18)
        self.tree.pack(fill=tk.BOTH, expand=True, side=tk.LEFT)
        
        scrollbar = ttk.Scrollbar(results_frame, orient=tk.VERTICAL, command=self.tree.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.tree.configure(yscrollcommand=scrollbar.set)
        
        self.tree.heading("#0", text="File Name / Path")
        self.tree.column("#0", width=650)
        self.tree.heading("size", text="Size")
        self.tree.column("size", width=120)
        self.tree.heading("similarity", text="Similarity")
        self.tree.column("similarity", width=120)
        
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
            messagebox.showerror("Error", "C++ backend not found. Please compile duplicate_finder first.")
            return
        
        if self.scanning:
            messagebox.showinfo("Info", "Scan already in progress")
            return
        
        self.clear_results()
        self.scanning = True
        
        # Show progress bar
        self.progress_frame.pack(fill=tk.X, pady=5)
        self.progress_bar.start(10)
        
        # Run scan in separate thread
        thread = threading.Thread(target=self.run_scan, args=(directory,))
        thread.daemon = True
        thread.start()
        
        # Start timer
        self.start_time = time.time()
        self.update_progress()
    
    def update_progress(self):
        if self.scanning:
            elapsed = int(time.time() - self.start_time)
            mins, secs = divmod(elapsed, 60)
            self.status_var.set(f"Scanning... Elapsed time: {mins}m {secs}s")
            self.root.after(1000, self.update_progress)
    
    def run_scan(self, directory):
        try:
            result = subprocess.run(
                [self.cpp_executable, directory, "--similar"],
                capture_output=True,
                text=True,
                timeout=300
            )
            
            self.root.after(0, self.scan_complete, result)
            
        except subprocess.TimeoutExpired:
            self.root.after(0, lambda: messagebox.showerror("Error", "Scanning timed out after 5 minutes"))
            self.scanning = False
        except Exception as e:
            self.root.after(0, lambda: messagebox.showerror("Error", f"Failed to run C++ backend: {e}"))
            self.scanning = False
    
    def scan_complete(self, result):
        self.scanning = False
        self.progress_bar.stop()
        self.progress_frame.pack_forget()
        
        if result.returncode != 0:
            messagebox.showerror("Error", f"C++ backend error:\n{result.stderr}")
            self.status_var.set("Scan failed")
            return
        
        # Show progress from stderr
        if result.stderr:
            for line in result.stderr.split('\n'):
                if line.strip() and any(keyword in line for keyword in ["Processed", "Finding", "Done", "Calculating"]):
                    self.status_var.set(line.strip())
                    self.root.update()
        
        self.parse_results(result.stdout)
    
    def parse_results(self, output):
        groups = []
        current_group = []
        group_type = "EXACT"
        group_similarity = 1.0
        
        for line in output.strip().split('\n'):
            line = line.strip()
            if not line:
                continue
                
            if line.startswith("EXACT|") or line.startswith("SIMILAR|"):
                parts = line.split('|')
                group_type = parts[0]
                group_similarity = float(parts[1]) if len(parts) > 1 else 1.0
            elif line == "---GROUP---":
                if current_group:
                    groups.append((group_type, group_similarity, current_group))
                    current_group = []
            else:
                parts = line.split('|')
                file_path = parts[0]
                file_sim = float(parts[1]) if len(parts) > 1 else group_similarity
                current_group.append((file_path, file_sim))
        
        if current_group:
            groups.append((group_type, group_similarity, current_group))
        
        self.duplicate_groups = groups
        self.display_results()
    
    def display_results(self):
        if not self.duplicate_groups:
            messagebox.showinfo("No Duplicates", "No duplicate or similar files found!")
            self.status_var.set("No duplicates found")
            return
        
        exact_count = 0
        similar_count = 0
        
        for group_type, group_similarity, group in self.duplicate_groups:
            if len(group) > 1:
                if group_type == "EXACT":
                    exact_count += 1
                    label = f"🔴 Exact Duplicates #{exact_count} ({len(group)} files)"
                    group_sim_display = "100%"
                else:
                    similar_count += 1
                    avg_sim = sum(sim for _, sim in group) / len(group)
                    label = f"🟡 Similar Files #{similar_count} ({len(group)} files) - Similarity: {avg_sim*100:.0f}%"
                    group_sim_display = f"{avg_sim*100:.0f}%"
                
                group_item = self.tree.insert("", "end", text=label, 
                                            values=("", group_sim_display),
                                            tags=('group',))
                
                for file_path, file_sim in group:
                    try:
                        file_size = os.path.getsize(file_path) if os.path.exists(file_path) else 0
                        size_str = f"{file_size / 1024:.1f} KB" if file_size > 0 else "N/A"
                    except:
                        size_str = "N/A"
                    
                    if group_type == "SIMILAR":
                        sim_str = f"{file_sim*100:.0f}%"
                    else:
                        sim_str = "100%"
                    
                    filename = os.path.basename(file_path)
                    
                    file_item = self.tree.insert(group_item, "end", 
                                               text=f"  📄 {filename}", 
                                               values=(size_str, sim_str))
                
                self.tree.item(group_item, open=True)
        
        self.tree.tag_configure('group', font=('TkDefaultFont', 10, 'bold'))
        
        status_msg = f"✅ Found {exact_count} exact duplicate groups"
        if similar_count > 0:
            status_msg += f" and {similar_count} similar file groups"
        self.status_var.set(status_msg)
    
    def delete_all_duplicates(self):
        if not self.duplicate_groups:
            messagebox.showinfo("Info", "No duplicates to delete")
            return
        
        total_to_delete = 0
        for group_type, _, group in self.duplicate_groups:
            if group_type == "EXACT":
                total_to_delete += len(group) - 1
        
        if total_to_delete == 0:
            messagebox.showinfo("Info", "No exact duplicates to delete")
            return
        
        confirm = messagebox.askyesno(
            "⚠️ Confirm Deletion",
            f"This will permanently delete {total_to_delete} EXACT duplicate files.\n\n"
            "✅ One original will be kept for each group.\n"
            "⚠️ Similar files will NOT be deleted.\n\n"
            "Are you sure you want to continue?"
        )
        
        if not confirm:
            return
        
        deleted_count = 0
        errors = []
        
        for group_type, _, group in self.duplicate_groups:
            if group_type == "EXACT" and len(group) > 1:
                for file_path, _ in group[1:]:
                    try:
                        os.remove(file_path)
                        deleted_count += 1
                    except Exception as e:
                        errors.append(f"{os.path.basename(file_path)}: {e}")
        
        if errors:
            error_msg = f"✅ Deleted {deleted_count} files\n\n⚠️ Errors encountered:\n\n" + "\n".join(errors[:5])
            if len(errors) > 5:
                error_msg += f"\n\n... and {len(errors) - 5} more errors"
            messagebox.showwarning("Deletion Complete with Errors", error_msg)
        else:
            messagebox.showinfo("✅ Deletion Complete", 
                              f"Successfully deleted {deleted_count} duplicate files!")
        
        self.status_var.set(f"Deleted {deleted_count} files")
        self.scan_duplicates()
    
    def clear_results(self):
        for item in self.tree.get_children():
            self.tree.delete(item)
        self.duplicate_groups = []
        self.status_var.set("Results cleared")

if __name__ == "__main__":
    root = tk.Tk()
    app = DuplicateFinderGUI(root)
    root.mainloop()