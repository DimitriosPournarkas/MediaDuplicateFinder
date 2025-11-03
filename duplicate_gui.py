import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import subprocess
import os
import threading
import time
import queue

class DuplicateFinderGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Duplicate File Finder - C++ Backend")
        self.root.geometry("1200x750")
        
        current_dir = os.path.dirname(os.path.abspath(__file__))
        if os.name == 'nt':
            self.cpp_executable = os.path.join(current_dir, "duplicate_finder.exe")
        else:
            self.cpp_executable = os.path.join(current_dir, "duplicate_finder")
        print(f"C++ executable path: {self.cpp_executable}")
        
        self.scanning = False
        self.process = None
        
        # Variables for live output processing
        self.stdout_queue = queue.Queue()
        self.stderr_queue = queue.Queue()
        self.stdout_buffer = ""
        self.stderr_buffer = ""
        
        # Progress tracking variables
        self.total_files = 0
        self.total_comparisons = 0
        self.processed_files = 0
        self.processed_comparisons = 0
        self.total_work = 0
        
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
        
        self.scan_button = ttk.Button(button_frame, text="Scan Duplicates & Similar Files", 
                  command=self.scan_duplicates, width=30)
        self.scan_button.pack(side=tk.LEFT, padx=5)
        
        self.cancel_button = ttk.Button(button_frame, text="Cancel Scan", 
                  command=self.cancel_scan, width=15, state=tk.DISABLED)
        self.cancel_button.pack(side=tk.LEFT, padx=5)
        
        ttk.Button(button_frame, text="Delete All Exact Duplicates", 
                  command=self.delete_all_duplicates, width=25).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Delete Selected Group", 
                  command=self.delete_selected_group, width=20).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Clear Results", 
                  command=self.clear_results).pack(side=tk.LEFT, padx=5)
        
        # Filter frame
        filter_frame = ttk.Frame(main_frame)
        filter_frame.pack(fill=tk.X, pady=5)
        
        ttk.Label(filter_frame, text="Show:").pack(side=tk.LEFT, padx=5)
        
        self.filter_var = tk.StringVar(value="all")
        ttk.Radiobutton(filter_frame, text="All Files", variable=self.filter_var, 
                       value="all", command=self.apply_filter).pack(side=tk.LEFT, padx=5)
        ttk.Radiobutton(filter_frame, text="Exact Duplicates Only", variable=self.filter_var, 
                       value="exact", command=self.apply_filter).pack(side=tk.LEFT, padx=5)
        ttk.Radiobutton(filter_frame, text="Similar Files Only", variable=self.filter_var, 
                       value="similar", command=self.apply_filter).pack(side=tk.LEFT, padx=5)
        
        # Statistics frame
        stats_frame = ttk.LabelFrame(main_frame, text="Statistics", padding="5")
        stats_frame.pack(fill=tk.X, pady=5)
        
        self.stats_var = tk.StringVar(value="No scan performed yet")
        stats_label = ttk.Label(stats_frame, textvariable=self.stats_var, font=('TkDefaultFont', 10))
        stats_label.pack()
        
        # Progress bar
        self.progress_frame = ttk.Frame(main_frame)
        self.progress_frame.pack(fill=tk.X, pady=5)
        
        self.progress_bar = ttk.Progressbar(self.progress_frame, mode='determinate', maximum=100)
        self.progress_bar.pack(fill=tk.X, pady=5)
        self.progress_frame.pack_forget()  # Hide initially
        
        results_frame = ttk.LabelFrame(main_frame, text="Results", padding="5")
        results_frame.pack(fill=tk.BOTH, expand=True, pady=10)
        
        # Treeview with path column
        self.tree = ttk.Treeview(results_frame, columns=("path", "size", "similarity"), 
                                show="tree headings", height=20)
        self.tree.pack(fill=tk.BOTH, expand=True, side=tk.LEFT)
        
        scrollbar = ttk.Scrollbar(results_frame, orient=tk.VERTICAL, command=self.tree.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.tree.configure(yscrollcommand=scrollbar.set)
        
        self.tree.heading("#0", text="File Name")
        self.tree.column("#0", width=350)
        self.tree.heading("path", text="Directory")
        self.tree.column("path", width=400)
        self.tree.heading("size", text="Size")
        self.tree.column("size", width=100)
        self.tree.heading("similarity", text="Similarity")
        self.tree.column("similarity", width=100)
        
        # Bind right-click for context menu
        self.tree.bind("<Button-3>", self.show_context_menu)
        
        self.status_var = tk.StringVar(value="Ready - Select directory and click Scan")
        status_bar = ttk.Label(main_frame, textvariable=self.status_var, relief=tk.SUNKEN)
        status_bar.pack(fill=tk.X, pady=5)
        
        self.duplicate_groups = []
        self.group_items = {}  # Map tree items to group data
        
    def show_context_menu(self, event):
        """Show context menu on right-click"""
        item = self.tree.identify_row(event.y)
        if item:
            self.tree.selection_set(item)
            menu = tk.Menu(self.root, tearoff=0)
            
            # Check if it's a group header or file
            if item in self.group_items:
                menu.add_command(label="Delete This Group", command=self.delete_selected_group)
            else:
                menu.add_command(label="Open File Location", command=lambda: self.open_file_location(item))
                menu.add_command(label="Delete This File", command=lambda: self.delete_single_file(item))
            
            menu.post(event.x_root, event.y_root)
    
    def open_file_location(self, item):
        """Open the folder containing the file"""
        file_path = self.tree.set(item, "path")
        if file_path and os.path.exists(file_path):
            if os.name == 'nt':  # Windows
                os.startfile(os.path.dirname(file_path))
            else:  # macOS/Linux
                subprocess.run(['xdg-open', os.path.dirname(file_path)])
    
    def delete_single_file(self, item):
        """Delete a single file"""
        file_path = self.tree.set(item, "path")
        filename = self.tree.item(item, "text")
        
        confirm = messagebox.askyesno(
            "Confirm Deletion",
            f"Delete this file?\n\n{filename}\n\nFrom: {os.path.dirname(file_path)}"
        )
        
        if confirm:
            try:
                os.remove(file_path)
                self.tree.delete(item)
                messagebox.showinfo("Success", "File deleted successfully")
                self.update_statistics()  # Update stats after deletion
            except Exception as e:
                messagebox.showerror("Error", f"Could not delete file: {e}")
        
    def browse_directory(self):
        directory = filedialog.askdirectory(title="Select Directory to Scan")
        if directory:
            self.dir_var.set(directory)
            self.status_var.set(f"Selected directory: {directory}")
    
    def cancel_scan(self):
        """Cancel the running scan"""
        if self.scanning and self.process:
            try:
                self.process.terminate()
                self.scanning = False
                self.progress_frame.pack_forget()
                self.status_var.set("Scan cancelled by user")
                self.scan_button.config(state=tk.NORMAL)
                self.cancel_button.config(state=tk.DISABLED)
            except Exception as e:
                messagebox.showerror("Error", f"Failed to cancel scan: {e}")
    
    def update_progress(self):
        if self.scanning:
            total_done = self.processed_files + self.processed_comparisons
            
            if self.total_work > 0:
                percentage = min(100, int(100 * total_done / self.total_work))
                self.progress_bar['value'] = percentage
                
                elapsed = time.time() - self.start_time
                status = f"📁 {total_done}/{self.total_work} Scans ({percentage}%) | ⏱️ {elapsed:.1f}s"
                self.status_var.set(status)
            else:
                self.status_var.set("Scanning...")
            
            self.root.after(1000, self.update_progress)
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
        self.scan_button.config(state=tk.DISABLED)
        self.cancel_button.config(state=tk.NORMAL)
        
        # RESET buffers for new scan
        self.stdout_buffer = ""
        self.stderr_buffer = ""
        
        # Reset progress counters
        self.total_files = 0
        self.total_comparisons = 0
        self.processed_files = 0
        self.processed_comparisons = 0
        self.total_work = 0
        
        # Show progress bar
        self.progress_frame.pack(fill=tk.X, pady=5)
        self.progress_bar['value'] = 0
        
        # Run scan in separate thread
        thread = threading.Thread(target=self.run_scan, args=(directory,))
        thread.daemon = True
        thread.start()
        
        # Start timer and progress updates
        self.start_time = time.time()
        self.update_progress()
    
    def run_scan(self, directory):
        """Run C++ scan with live progress updates"""
        try:
            # Start process with live output
            self.process = subprocess.Popen(
                [self.cpp_executable, directory, "--similar"],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1,
                universal_newlines=True
            )
            
            # Start threads to read stdout and stderr in real-time
            stdout_thread = threading.Thread(target=self.read_stdout)
            stderr_thread = threading.Thread(target=self.read_stderr)
            
            stdout_thread.daemon = True
            stderr_thread.daemon = True
            
            stdout_thread.start()
            stderr_thread.start()
            
            # Start processing the output queues in GUI thread
            self.process_queues()
            
        except Exception as e:
            self.root.after(0, lambda: messagebox.showerror("Error", f"Failed to run C++ backend: {e}"))
            self.scanning = False
            self.scan_button.config(state=tk.NORMAL)
            self.cancel_button.config(state=tk.DISABLED)
    
    def read_stdout(self):
        """Read stdout in real-time for results"""
        try:
            for line in iter(self.process.stdout.readline, ''):
                if line.strip():
                    self.stdout_queue.put(line.strip())
        except Exception as e:
            print(f"Error reading stdout: {e}")

    def read_stderr(self):
        """Read stderr in real-time for progress updates"""
        try:
            for line in iter(self.process.stderr.readline, ''):
                if line.strip():
                    self.stderr_queue.put(line.strip())
        except Exception as e:
            print(f"Error reading stderr: {e}")

    def process_queues(self):
        """Process output queues in the main GUI thread"""
        # Process all stderr messages (progress updates)
        while not self.stderr_queue.empty():
            try:
                line = self.stderr_queue.get_nowait()
                self.process_stderr_line(line)
            except queue.Empty:
                break
        
        # Process all stdout messages (results)
        while not self.stdout_queue.empty():
            try:
                line = self.stdout_queue.get_nowait()
                self.stdout_buffer += line + "\n"
            except queue.Empty:
                break
        
        # Check if process is still running
        if self.process.poll() is None:
            # Process still running, check again in 100ms
            self.root.after(100, self.process_queues)
        else:
            # Process finished, complete the scan
            self.root.after(100, self.scan_complete_final)

    def process_stderr_line(self, line):
        """Process progress lines from C++ and update counters"""
        line = line.strip()
        
        if line.startswith("TOTAL_WORK:"):
            try:
                self.total_work = int(line.split(':')[1])
                self.processed_work = 0
            except:
                pass
            return
        
        try:
            if "Processed" in line and "/" in line:
                parts = line.split()
                for i, part in enumerate(parts):
                    if '/' in part:
                        nums = part.split('/')
                        current = int(nums[0])
                        
                        if "files" in line:
                            self.processed_files = current
                        elif "comparisons" in line:
                            self.processed_comparisons = current
                        break
                        
        except (ValueError, IndexError):
            pass
    
    def scan_complete_final(self):
        """Final completion handler after process ends"""
        self.scanning = False
        self.progress_bar.stop()
        self.progress_frame.pack_forget()
        self.scan_button.config(state=tk.NORMAL)
        self.cancel_button.config(state=tk.DISABLED)
        
        # Check for errors
        if self.process.returncode != 0:
            error_msg = f"C++ backend error (code {self.process.returncode})"
            if self.stderr_buffer:
                error_msg += f"\n\n{self.stderr_buffer}"
            messagebox.showerror("Error", error_msg)
            self.status_var.set("Scan failed")
            return
        
        # Parse and display results
        self.parse_results(self.stdout_buffer)
    
    def parse_results(self, output):
        self.status_var.set("Processing results...")
        self.root.update_idletasks()
        
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
    
    def calculate_wasted_space(self):
        """Calculate total space wasted by duplicates"""
        total_wasted = 0
        
        for group_type, group_similarity, group in self.duplicate_groups:
            if group_type == "EXACT" and len(group) > 1:
                # Get size of first file (all duplicates have same size)
                first_file = group[0][0]
                try:
                    file_size = os.path.getsize(first_file) if os.path.exists(first_file) else 0
                    # Wasted space = size * (number of duplicates - 1)
                    total_wasted += file_size * (len(group) - 1)
                except:
                    pass
        
        return total_wasted
    
    def update_statistics(self):
        """Update statistics display"""
        exact_count = 0
        similar_count = 0
        exact_files = 0
        similar_files = 0
        
        for group_type, group_similarity, group in self.duplicate_groups:
            if len(group) > 1:
                if group_type == "EXACT":
                    exact_count += 1
                    exact_files += len(group)
                else:
                    similar_count += 1
                    similar_files += len(group)
        
        wasted_space = self.calculate_wasted_space()
        
        stats_text = ""
        if exact_count > 0:
            stats_text += f"🔴 {exact_count} exact duplicate groups ({exact_files} files) | "
        if similar_count > 0:
            stats_text += f"🟡 {similar_count} similar file groups ({similar_files} files) | "
        
        if wasted_space > 0:
            if wasted_space > 1024 * 1024 * 1024:  # > 1 GB
                space_str = f"{wasted_space / (1024**3):.2f} GB"
            elif wasted_space > 1024 * 1024:  # > 1 MB
                space_str = f"{wasted_space / (1024**2):.2f} MB"
            else:
                space_str = f"{wasted_space / 1024:.2f} KB"
            stats_text += f"💾 Can free up: {space_str}"
        
        if not stats_text:
            stats_text = "No duplicates or similar files found"
        
        self.stats_var.set(stats_text)

    def apply_filter(self):
        """Apply filter to show only selected file types"""
        filter_value = self.filter_var.get()
        
        # Clear tree
        for item in self.tree.get_children():
            self.tree.delete(item)
        
        # Re-populate based on filter
        self.group_items = {}
        exact_count = 0
        similar_count = 0
        
        for idx, (group_type, group_similarity, group) in enumerate(self.duplicate_groups):
            # Apply filter
            if filter_value == "exact" and group_type != "EXACT":
                continue
            if filter_value == "similar" and group_type != "SIMILAR":
                continue
            
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
                                            values=("", "", group_sim_display),
                                            tags=('group',))
                
                self.group_items[group_item] = (group_type, group_similarity, group)
                
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
                    directory = os.path.dirname(file_path)
                    
                    file_item = self.tree.insert(group_item, "end", 
                                            text=f"  📄 {filename}", 
                                            values=(directory, size_str, sim_str))
                    self.tree.set(file_item, "path", file_path)
                
                self.tree.item(group_item, open=True)
        
        self.tree.tag_configure('group', font=('TkDefaultFont', 10, 'bold'))

    def display_results(self):
        if not self.duplicate_groups:
            messagebox.showinfo("No Duplicates", "No duplicate or similar files found!")
            self.status_var.set("No duplicates found")
            self.stats_var.set("No duplicates or similar files found")
            return
        
        self.status_var.set("Displaying results...")
        self.root.update_idletasks()
        
        exact_count = 0
        similar_count = 0
        
        self.group_items = {}
        
        for idx, (group_type, group_similarity, group) in enumerate(self.duplicate_groups):
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
                                            values=("", "", group_sim_display),
                                            tags=('group',))
                
                self.group_items[group_item] = (group_type, group_similarity, group)
                
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
                    directory = os.path.dirname(file_path)
                    
                    file_item = self.tree.insert(group_item, "end", 
                                            text=f"  📄 {filename}", 
                                            values=(directory, size_str, sim_str))
                    self.tree.set(file_item, "path", file_path)
                
                self.tree.item(group_item, open=True)
        
        self.tree.tag_configure('group', font=('TkDefaultFont', 10, 'bold'))
        
        # Update statistics
        self.update_statistics()
        
        # Final status
        status_msg = f"✅ Found {exact_count} exact duplicate groups"
        if similar_count > 0:
            status_msg += f" and {similar_count} similar file groups"
        self.status_var.set(status_msg)

    def get_file_priority(self, file_path):
        """Calculate priority for file retention"""
        directory = os.path.dirname(file_path)
        scan_directory = self.dir_var.get()
        
        if directory == scan_directory:
            return (0, "")
        
        return (1, directory)
    
    def show_deletion_preview(self, group):
        """Show which files will be kept/deleted"""
        sorted_group = sorted(group, key=lambda x: self.get_file_priority(x[0]))
        keep_file = sorted_group[0]
        delete_files = sorted_group[1:]
        
        preview_window = tk.Toplevel(self.root)
        preview_window.title("Deletion Preview")
        preview_window.geometry("700x500")
        
        main_frame = ttk.Frame(preview_window, padding="10")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # Keep section
        keep_frame = ttk.LabelFrame(main_frame, text="✅ File to KEEP", padding="10")
        keep_frame.pack(fill=tk.X, pady=5)
        
        keep_name = os.path.basename(keep_file[0])
        keep_dir = os.path.dirname(keep_file[0])
        try:
            keep_size = os.path.getsize(keep_file[0])
            keep_size_str = f"{keep_size / 1024:.1f} KB"
        except:
            keep_size_str = "N/A"
        
        ttk.Label(keep_frame, text=f"📄 {keep_name}", font=('TkDefaultFont', 10, 'bold')).pack(anchor=tk.W)
        ttk.Label(keep_frame, text=f"📁 {keep_dir}").pack(anchor=tk.W)
        ttk.Label(keep_frame, text=f"💾 {keep_size_str}").pack(anchor=tk.W)
        
        # Delete section
        delete_frame = ttk.LabelFrame(main_frame, text=f"❌ Files to DELETE ({len(delete_files)})", padding="10")
        delete_frame.pack(fill=tk.BOTH, expand=True, pady=5)
        
        # Scrollable list
        list_frame = ttk.Frame(delete_frame)
        list_frame.pack(fill=tk.BOTH, expand=True)
        
        scrollbar = ttk.Scrollbar(list_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        delete_list = tk.Listbox(list_frame, yscrollcommand=scrollbar.set, height=15)
        delete_list.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.config(command=delete_list.yview)
        
        total_delete_size = 0
        for file_path, _ in delete_files:
            filename = os.path.basename(file_path)
            directory = os.path.dirname(file_path)
            try:
                file_size = os.path.getsize(file_path)
                size_str = f"{file_size / 1024:.1f} KB"
                total_delete_size += file_size
            except:
                size_str = "N/A"
            
            delete_list.insert(tk.END, f"📄 {filename} ({size_str}) - {directory}")
        
        # Space to free
        if total_delete_size > 0:
            if total_delete_size > 1024 * 1024 * 1024:
                space_str = f"{total_delete_size / (1024**3):.2f} GB"
            elif total_delete_size > 1024 * 1024:
                space_str = f"{total_delete_size / (1024**2):.2f} MB"
            else:
                space_str = f"{total_delete_size / 1024:.2f} KB"
            
            space_label = ttk.Label(delete_frame, text=f"💾 Space to free: {space_str}", 
                                   font=('TkDefaultFont', 10, 'bold'))
            space_label.pack(pady=5)
        
        # Buttons
        button_frame = ttk.Frame(main_frame)
        button_frame.pack(fill=tk.X, pady=10)
        
        ttk.Button(button_frame, text="Close", command=preview_window.destroy).pack(side=tk.RIGHT, padx=5)
        
        return True
    
    def delete_selected_group(self):
        """Delete all duplicates in the selected group"""
        selection = self.tree.selection()
        if not selection:
            messagebox.showinfo("Info", "Please select a duplicate group to delete")
            return
        
        selected_item = selection[0]
        
        # Check if it's a group header
        if selected_item not in self.group_items:
            messagebox.showinfo("Info", "Please select a group header (not a single file)")
            return
        
        group_type, group_similarity, group = self.group_items[selected_item]
        
        if group_type != "EXACT":
            confirm = messagebox.askyesno(
                "⚠️ Warning",
                "This is a SIMILAR files group, not exact duplicates.\n\n"
                "Deleting similar files may remove files you want to keep!\n\n"
                "Continue anyway?"
            )
            if not confirm:
                return
        
        # Show preview window
        self.show_deletion_preview(group)
        
        # Sort by priority
        sorted_group = sorted(group, key=lambda x: self.get_file_priority(x[0]))
        keep_file = sorted_group[0]
        delete_files = sorted_group[1:]
        
        confirm_msg = f"Delete {len(delete_files)} files from this group?\n\n"
        confirm_msg += f"✓ Keep: {os.path.basename(keep_file[0])}\n"
        confirm_msg += f"   ({os.path.dirname(keep_file[0])})\n\n"
        confirm_msg += f"✗ Delete {len(delete_files)} files"
        
        confirm = messagebox.askyesno("Confirm Deletion", confirm_msg)
        
        if not confirm:
            return
        
        deleted_count = 0
        errors = []
        
        for file_path, _ in delete_files:
            try:
                os.remove(file_path)
                deleted_count += 1
            except Exception as e:
                errors.append(f"{os.path.basename(file_path)}: {e}")
        
        if errors:
            messagebox.showwarning("Deletion Complete with Errors", 
                                 f"Deleted {deleted_count} files\n\nErrors:\n" + "\n".join(errors[:3]))
        else:
            messagebox.showinfo("Success", f"Deleted {deleted_count} files from group")
        
        # Remove group from tree
        self.tree.delete(selected_item)
        del self.group_items[selected_item]
        
        # Update statistics
        self.update_statistics()
    
    def delete_all_duplicates(self):
        if not self.duplicate_groups:
            messagebox.showinfo("Info", "No duplicates to delete")
            return
        
        total_to_delete = 0
        files_to_delete = []
        keep_decisions = []
        total_space_to_free = 0
        
        for group_type, group_similarity, group in self.duplicate_groups:
            if group_type == "EXACT" and len(group) > 1:
                sorted_group = sorted(group, key=lambda x: self.get_file_priority(x[0]))
                
                keep_file = sorted_group[0]
                delete_files = sorted_group[1:]
                
                keep_decisions.append(f"✓ Keep: {os.path.basename(keep_file[0])} ({os.path.dirname(keep_file[0])})")
                
                for file_path, _ in delete_files:
                    files_to_delete.append(file_path)
                    total_to_delete += 1
                    try:
                        file_size = os.path.getsize(file_path)
                        total_space_to_free += file_size
                    except:
                        pass
        
        if total_to_delete == 0:
            messagebox.showinfo("Info", "No exact duplicates to delete")
            return
        
        # Format space to free
        if total_space_to_free > 1024 * 1024 * 1024:
            space_str = f"{total_space_to_free / (1024**3):.2f} GB"
        elif total_space_to_free > 1024 * 1024:
            space_str = f"{total_space_to_free / (1024**2):.2f} MB"
        else:
            space_str = f"{total_space_to_free / 1024:.2f} KB"
        
        confirmation_text = f"This will permanently delete {total_to_delete} EXACT duplicate files.\n"
        confirmation_text += f"💾 Space to free: {space_str}\n\n"
        confirmation_text += "Keeping decisions:\n" + "\n".join(keep_decisions[:5])
        if len(keep_decisions) > 5:
            confirmation_text += f"\n... and {len(keep_decisions) - 5} more groups"
        
        confirmation_text += "\n\n✅ One original kept per group\n⚠️ Similar files will NOT be deleted"
        
        confirm = messagebox.askyesno("⚠️ Confirm Deletion", confirmation_text)
        
        if not confirm:
            return
        
        deleted_count = 0
        errors = []
        
        for file_path in files_to_delete:
            try:
                os.remove(file_path)
                deleted_count += 1
            except Exception as e:
                errors.append(f"{os.path.basename(file_path)}: {e}")
        
        if errors:
            error_msg = f"✅ Deleted {deleted_count} files\n\n⚠️ Errors:\n\n" + "\n".join(errors[:5])
            if len(errors) > 5:
                error_msg += f"\n\n... and {len(errors) - 5} more errors"
            messagebox.showwarning("Deletion Complete with Errors", error_msg)
        else:
            messagebox.showinfo("✅ Deletion Complete", 
                              f"Successfully deleted {deleted_count} files!\n💾 Freed up: {space_str}")
        
        self.status_var.set(f"Deleted {deleted_count} files, freed {space_str}")
        self.scan_duplicates()
    
    def clear_results(self):
        for item in self.tree.get_children():
            self.tree.delete(item)
        self.duplicate_groups = []
        self.group_items = {}
        self.status_var.set("Results cleared")
        self.stats_var.set("No scan performed yet")

if __name__ == "__main__":
    root = tk.Tk()
    app = DuplicateFinderGUI(root)
    root.mainloop()