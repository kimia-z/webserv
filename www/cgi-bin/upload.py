#!/usr/bin/env python3
import os
import html
import sys
import io

print("Content-Type: text/html\n")

# dir where the files will be uploaded to
UPLOAD_DIR = os.environ.get("UPLOAD_DIR", "/tmp/uploads")

# Convert to absolute path if it's relative
if not os.path.isabs(UPLOAD_DIR):
	# Get the directory where this script is located
	script_dir = os.path.dirname(os.path.abspath(__file__))
	# Go up one level to get to www directory
	www_dir = os.path.dirname(script_dir)
	# If UPLOAD_DIR starts with 'www/', remove it to avoid duplication
	if UPLOAD_DIR.startswith('www/'):
		UPLOAD_DIR = UPLOAD_DIR[4:]  # Remove 'www/' prefix
	# Create absolute path
	UPLOAD_DIR = os.path.join(www_dir, UPLOAD_DIR)

# pull the limit data from env
try:
	MAX_FILE_SIZE = int(os.environ.get("MAX_FILE_SIZE", "1048576"))
except ValueError:
	MAX_FILE_SIZE = 1048576  # or set the default of 10 MB

# making sure the upload exists
try:
	os.makedirs(UPLOAD_DIR, exist_ok=True)
except OSError as e:
	print(f"<h2>Error: Folder {html.escape(UPLOAD_DIR)} cannot be created: {html.escape(str(e))}</h2>")
	exit()

# Simple multipart form parser
def parse_multipart_form_data():
	content_type = os.environ.get("CONTENT_TYPE", "")
	if not content_type.startswith("multipart/form-data"):
		raise Exception("Invalid content type")
	
	boundary = content_type.split("boundary=")[1]
	content_length = int(os.environ.get("CONTENT_LENGTH", "0"))
	
	if content_length == 0:
		raise Exception("No content")
	
	# Read the raw input as binary
	raw_data = sys.stdin.buffer.read(content_length)
	
	# Find the file part
	boundary_bytes = ("--" + boundary).encode('utf-8')
	parts = raw_data.split(boundary_bytes)
	for part in parts:
		part_str = part.decode('utf-8', errors='ignore')
		if "name=\"file\"" in part_str and "filename=" in part_str:
			# Extract filename
			filename_line = [line for line in part_str.split("\r\n") if "filename=" in line][0]
			filename = filename_line.split("filename=")[1].strip('"')
			
			# Extract file data (skip headers)
			file_data_start = part.find(b"\r\n\r\n") + 4
			file_data = part[file_data_start:]
			
			# Remove trailing boundary
			if file_data.endswith(b"\r\n"):
				file_data = file_data[:-2]
			
			return filename, file_data
	
	raise Exception("No file found in form data")

try:
	filename, file_data = parse_multipart_form_data()
	
	if filename:
		if len(file_data) > MAX_FILE_SIZE:
			print(f"""
			<html>
			<head><meta charset="UTF-8"><title>Error</title></head>
			<body>
			<h2>File too big</h2>
			<p>Max size is {MAX_FILE_SIZE} bytes.</p>
			<br>
			<a href="/upload.html">upload page</a>
			</body>
			</html>
			""")
		else:
			# save the file
			saved_file_name = filename
			saved_file_path = os.path.join(UPLOAD_DIR, saved_file_name)

			base, extension = os.path.splitext(saved_file_name)
			counter = 1
			while os.path.exists(saved_file_path):
				saved_file_path = os.path.join(UPLOAD_DIR, f"{base}_{counter}{extension}")
				counter += 1

			with open(saved_file_path, "wb") as f:
				# Write file data as binary (it's already binary from the multipart parsing)
				f.write(file_data)

			final_filename = os.path.basename(saved_file_path)

			print(f"""
			<html>
			<head><meta charset="UTF-8"><title>Success</title></head>
			<body>
			<h2>File has been saved successfully!</h2>
			<p>File name: {html.escape(final_filename)}</p>
			<p>Size: {len(file_data)} bytes</p>
			<br>
			<a href="/index.html">homepage</a>
			</body>
			</html>
			""")
	else:
		print("""
		<html>
		<head><meta charset="UTF-8"><title>Error</title></head>
		<body>
		<h2>Error: No file attached</h2>
		<p>Please select a file to upload.</p>
		<br>
		<a href="/upload.html">upload page</a>
		</body>
		</html>
		""")
		
except Exception as e:
	print(f"""
	<html>
	<head><meta charset="UTF-8"><title>Error</title></head>
	<body>
	<h2>Error: Failed to parse form data</h2>
	<p>Error: {html.escape(str(e))}</p>
	<br>
	<a href="/upload.html">upload page</a>
	</body>
	</html>
	""")