import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import threading
import subprocess
import os
<<<<<<< Updated upstream
=======
import threading
import time
>>>>>>> Stashed changes

class DuplicateFinderGUI:
    def __init__(self, root):
        self.root = root
<<<<<<< Updated upstream
        self.root.title("Duplicate File Finder")
        self.root.geometry("1000x700")
=======
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
>>>>>>> Stashed changes
        
        self.current_directory = ""
        self.last_scanned_directory = None
        self.is_first_scan = True
        self.duplicate_groups = []
        self.similar_groups = []
        
        self.setup_ui()
    
    def setup_ui(self):
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        dir_frame = ttk.Frame(main_frame)
        dir_frame.grid(row=0, column=0, columnspan=3, sticky=(tk.W, tk.E), pady=5)
        
        ttk.Label(dir_frame, text="Select Directory:").grid(row=0, column=0, sticky=tk.W)
        self.dir_var = tk.StringVar()
        self.dir_entry = ttk.Entry(dir_frame, textvariable=self.dir_var, width=60)
        self.dir_entry.grid(row=0, column=1, padx=5)
        
        ttk.Button(dir_frame, text="Browse", command=self.browse_directory).grid(row=0, column=2)
        
<<<<<<< Updated upstream
        # Action buttons
=======
>>>>>>> Stashed changes
        button_frame = ttk.Frame(main_frame)
        button_frame.grid(row=1, column=0, columnspan=3, pady=10, sticky=tk.W)
        
<<<<<<< Updated upstream
        self.scan_button = ttk.Button(button_frame, text="Scan Duplicates", command=self.start_scan)
        self.scan_button.pack(side=tk.LEFT, padx=(0, 10))
        
        self.delete_all_button = ttk.Button(button_frame, text="Delete All Duplicates", 
                                          command=self.delete_all_duplicates, state=tk.DISABLED)
        self.delete_all_button.pack(side=tk.LEFT, padx=(0, 10))
        
        self.delete_selected_button = ttk.Button(button_frame, text="Delete Selected", 
                                               command=self.delete_selected, state=tk.DISABLED)
        self.delete_selected_button.pack(side=tk.LEFT, padx=(0, 10))
        
        ttk.Button(button_frame, text="Clear Results", command=self.clear_results).pack(side=tk.LEFT)
        
        # Progress bar
        self.progress = ttk.Progressbar(main_frame, mode='indeterminate')
        self.progress.grid(row=2, column=0, columnspan=3, sticky=(tk.W, tk.E), pady=5)
        self.progress.grid_remove()
        
        # Status label
        self.status_var = tk.StringVar(value="Ready")
        status_label = ttk.Label(main_frame, textvariable=self.status_var)
        status_label.grid(row=3, column=0, columnspan=3, sticky=tk.W, pady=5)
=======
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
>>>>>>> Stashed changes
        
        # Results area with Notebook for tabs
        notebook = ttk.Notebook(main_frame)
        notebook.grid(row=4, column=0, columnspan=3, sticky=(tk.W, tk.E, tk.N, tk.S), pady=10)
        
        # Exact duplicates tab
        exact_frame = ttk.Frame(notebook, padding="5")
        notebook.add(exact_frame, text="Exact Duplicates")
        
        self.exact_tree = ttk.Treeview(exact_frame, columns=("File", "Size", "Path"), show="tree headings")
        self.exact_tree.heading("#0", text="Duplicate Group")
        self.exact_tree.heading("File", text="File Name")
        self.exact_tree.heading("Size", text="Size")
        self.exact_tree.heading("Path", text="Full Path")
        
        self.exact_tree.column("#0", width=150)
        self.exact_tree.column("File", width=200)
        self.exact_tree.column("Size", width=100)
        self.exact_tree.column("Path", width=400)
        
        exact_scrollbar = ttk.Scrollbar(exact_frame, orient="vertical", command=self.exact_tree.yview)
        self.exact_tree.configure(yscrollcommand=exact_scrollbar.set)
        
        self.exact_tree.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        exact_scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        
        # Similar files tab
        similar_frame = ttk.Frame(notebook, padding="5")
        notebook.add(similar_frame, text="Similar Files")
        
        self.similar_tree = ttk.Treeview(similar_frame, columns=("File", "Size", "Similarity", "Path"), show="tree headings")
        self.similar_tree.heading("#0", text="Similar Group")
        self.similar_tree.heading("File", text="File Name")
        self.similar_tree.heading("Size", text="Size")
        self.similar_tree.heading("Similarity", text="Similarity")
        self.similar_tree.heading("Path", text="Full Path")
        
        self.similar_tree.column("#0", width=150)
        self.similar_tree.column("File", width=200)
        self.similar_tree.column("Size", width=100)
        self.similar_tree.column("Similarity", width=80)
        self.similar_tree.column("Path", width=400)
        
        similar_scrollbar = ttk.Scrollbar(similar_frame, orient="vertical", command=self.similar_tree.yview)
        self.similar_tree.configure(yscrollcommand=similar_scrollbar.set)
        
        self.similar_tree.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        similar_scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        
        # Configure grid weights
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(0, weight=1)
        main_frame.rowconfigure(4, weight=1)
        exact_frame.columnconfigure(0, weight=1)
        exact_frame.rowconfigure(0, weight=1)
        similar_frame.columnconfigure(0, weight=1)
        similar_frame.rowconfigure(0, weight=1)
        
        # Expand all groups by default
        self.exact_tree.bind("<<TreeviewOpen>>", self.on_treeview_open)
        self.similar_tree.bind("<<TreeviewOpen>>", self.on_treeview_open)
    
    def browse_directory(self):
        directory = filedialog.askdirectory()
        if directory:
            self.dir_var.set(directory)
            self.current_directory = directory
            
            if self.is_first_scan:
                self.is_first_scan = False
                self.start_scan()
            else:
                self.scan_button.config(state=tk.NORMAL)
                self.scan_button.config(text=f"Scan {os.path.basename(directory)}")
    
    def start_scan(self):
        if not self.current_directory:
            messagebox.showwarning("Warning", "Please select a directory first.")
            return
        
<<<<<<< Updated upstream
        self.scan_button.config(state=tk.DISABLED)
        self.delete_all_button.config(state=tk.DISABLED)
        self.delete_selected_button.config(state=tk.DISABLED)
        self.progress.grid()
        self.progress.start()
        self.status_var.set("Scanning for duplicates...")
        
        self.clear_results()
        
        scan_thread = threading.Thread(target=self.run_scan)
        scan_thread.daemon = True
        scan_thread.start()
    
    def run_scan(self):
=======
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
>>>>>>> Stashed changes
        try:
            result = subprocess.run(
                ["./duplicate_finder", self.current_directory],
                capture_output=True,
                text=True,
                cwd=os.path.dirname(os.path.abspath(__file__))
            )
            
<<<<<<< Updated upstream
            if result.returncode != 0:
                self.root.after(0, self.on_scan_error, result.stderr)
            else:
                self.root.after(0, self.on_scan_complete, result.stdout, "")
                
        except Exception as e:
            self.root.after(0, self.on_scan_error, str(e))
    
    def on_scan_complete(self, stdout, stderr):
        self.progress.stop()
        self.progress.grid_remove()
        self.scan_button.config(state=tk.NORMAL)
        self.last_scanned_directory = self.current_directory
        
        if not stdout.strip():
            self.status_var.set("No duplicates found.")
            messagebox.showinfo("Info", "No duplicates found.")
            return
        
        self.parse_results(stdout)
        self.status_var.set(f"Scan complete. Found {len(self.duplicate_groups)} duplicate groups.")
        
        # Enable delete buttons if duplicates found
        if self.duplicate_groups or self.similar_groups:
            self.delete_all_button.config(state=tk.NORMAL)
            self.delete_selected_button.config(state=tk.NORMAL)
    
    def on_scan_error(self, error_msg):
        self.progress.stop()
        self.progress.grid_remove()
        self.scan_button.config(state=tk.NORMAL)
        self.status_var.set("Scan failed")
        messagebox.showerror("Error", f"Scan failed:\n{error_msg}")
    
    def parse_results(self, output):
        lines = output.strip().split('\n')
        current_exact_group = None
        current_similar_group = None
        exact_group_id = 1
        similar_group_id = 1
=======
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
>>>>>>> Stashed changes
        
        for line in lines:
            line = line.strip()
<<<<<<< Updated upstream
            
            if line == "---EXACT_GROUP---":
                current_exact_group = None
                exact_group_id += 1
            elif line == "---SIMILAR_GROUP---":
                current_similar_group = None
                similar_group_id += 1
            elif line.startswith("Found similar group"):
                # New similar group
                current_similar_group = self.similar_tree.insert("", "end", text=f"Similar Group {similar_group_id}", values=("", "", "", ""))
                self.similar_groups.append(current_similar_group)
            elif line and current_similar_group and not line.startswith("Scanning"):
                # File in similar group
                if "(similarity:" in line:
                    file_part, similarity_part = line.split("(similarity:")
                    filename = file_part.strip()
                    similarity = similarity_part.replace(")", "").strip()
                    file_path = self.find_file_path(filename)
                    file_size = self.get_file_size(file_path) if file_path else ""
                    self.similar_tree.insert(current_similar_group, "end", text="", 
                                           values=(filename, file_size, similarity, file_path))
                else:
                    # First file in similar group (no similarity score)
                    filename = line
                    file_path = self.find_file_path(filename)
                    file_size = self.get_file_size(file_path) if file_path else ""
                    self.similar_tree.insert(current_similar_group, "end", text="", 
                                           values=(filename, file_size, "", file_path))
            elif line and not line.startswith("Scanning") and not line.startswith("Found"):
                # Exact duplicate file
                if not current_exact_group:
                    current_exact_group = self.exact_tree.insert("", "end", text=f"Duplicate Group {exact_group_id}", values=("", "", ""))
                    self.duplicate_groups.append(current_exact_group)
                
                filename = os.path.basename(line)
                file_size = self.get_file_size(line)
                self.exact_tree.insert(current_exact_group, "end", text="", 
                                     values=(filename, file_size, line))
        
        # Expand all groups by default
        for group in self.exact_tree.get_children():
            self.exact_tree.item(group, open=True)
        for group in self.similar_tree.get_children():
            self.similar_tree.item(group, open=True)
    
    def find_file_path(self, filename):
        # Simple implementation - you might want to improve this
        for root, dirs, files in os.walk(self.current_directory):
            if filename in files:
                return os.path.join(root, filename)
        return ""
    
    def get_file_size(self, file_path):
        try:
            size_bytes = os.path.getsize(file_path)
            if size_bytes < 1024:
                return f"{size_bytes} B"
            elif size_bytes < 1024 * 1024:
                return f"{size_bytes/1024:.1f} KB"
            else:
                return f"{size_bytes/(1024*1024):.1f} MB"
        except:
            return ""
    
    def on_treeview_open(self, event):
        # Keep groups expanded
        pass
=======
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
>>>>>>> Stashed changes
    
    def delete_all_duplicates(self):
        if not self.duplicate_groups and not self.similar_groups:
            messagebox.showinfo("Info", "No duplicates to delete.")
            return
        
<<<<<<< Updated upstream
        # For exact duplicates, keep one file from each group and delete the rest
        files_to_delete = []
        
        # Process exact duplicates
        for group in self.duplicate_groups:
            children = self.exact_tree.get_children(group)
            if len(children) > 1:
                # Keep first file, delete the rest
                files_to_delete.extend([self.exact_tree.set(child, "Path") for child in children[1:]])
        
        # Process similar files (ask user which ones to keep)
        for group in self.similar_groups:
            children = self.similar_tree.get_children(group)
            if len(children) > 1:
                # For now, just add all similar files (user should select manually)
                files_to_delete.extend([self.similar_tree.set(child, "Path") for child in children])
        
        if not files_to_delete:
            messagebox.showinfo("Info", "No duplicates selected for deletion.")
            return
        
        confirm = messagebox.askyesno("Confirm Delete", 
                                    f"Are you sure you want to delete {len(files_to_delete)} files?\n\n"
                                    "This action cannot be undone!")
        if confirm:
            self.delete_files(files_to_delete)
    
    def delete_selected(self):
        selected_exact = self.exact_tree.selection()
        selected_similar = self.similar_tree.selection()
        
        files_to_delete = []
        
        # Get selected files from exact duplicates
        for item in selected_exact:
            if self.exact_tree.parent(item):  # It's a file, not a group
                files_to_delete.append(self.exact_tree.set(item, "Path"))
        
        # Get selected files from similar files
        for item in selected_similar:
            if self.similar_tree.parent(item):  # It's a file, not a group
                files_to_delete.append(self.similar_tree.set(item, "Path"))
        
        if not files_to_delete:
            messagebox.showinfo("Info", "No files selected for deletion.")
            return
        
        confirm = messagebox.askyesno("Confirm Delete", 
                                    f"Are you sure you want to delete {len(files_to_delete)} files?\n\n"
                                    "This action cannot be undone!")
        if confirm:
            self.delete_files(files_to_delete)
    
    def delete_files(self, file_paths):
        success_count = 0
        fail_count = 0
        
        for file_path in file_paths:
            try:
                if os.path.exists(file_path):
                    os.remove(file_path)
                    success_count += 1
                else:
                    fail_count += 1
            except Exception as e:
                print(f"Error deleting {file_path}: {e}")
                fail_count += 1
        
        # Refresh results after deletion
        if success_count > 0:
            messagebox.showinfo("Delete Complete", 
                              f"Successfully deleted {success_count} files.\n"
                              f"Failed to delete {fail_count} files.")
            self.start_scan()  # Rescan to update results
        else:
            messagebox.showwarning("Delete Failed", "No files were deleted.")
=======
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
>>>>>>> Stashed changes
    
    def clear_results(self):
        for item in self.exact_tree.get_children():
            self.exact_tree.delete(item)
        for item in self.similar_tree.get_children():
            self.similar_tree.delete(item)
        self.duplicate_groups.clear()
        self.similar_groups.clear()
        self.delete_all_button.config(state=tk.DISABLED)
        self.delete_selected_button.config(state=tk.DISABLED)
        self.status_var.set("Ready")

def main():
    root = tk.Tk()
    app = DuplicateFinderGUI(root)
    root.mainloop()

if __name__ == "__main__":
    main()