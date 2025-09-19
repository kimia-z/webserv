#!/usr/bin/env python3
import cgi
import os
import html

print("Content-Type: text/html\n")

# # Debug info
# print("<h1>DEBUG INFO</h1>")
# print("<p>REQUEST_METHOD: " + os.environ.get('REQUEST_METHOD', 'NOT SET') + "</p>")
# print("<p>CONTENT_TYPE: " + os.environ.get('CONTENT_TYPE', 'NOT SET') + "</p>")
# print("<p>CONTENT_LENGTH: " + os.environ.get('CONTENT_LENGTH', 'NOT SET') + "</p>")

# Setup upload directory
UPLOAD_DIR = os.environ.get("UPLOAD_DIR", "upload")
if not os.path.isabs(UPLOAD_DIR):
	script_dir = os.path.dirname(os.path.abspath(__file__))
	www_dir = os.path.dirname(script_dir)
	if UPLOAD_DIR.startswith('www/'):
		UPLOAD_DIR = UPLOAD_DIR[4:] 
	UPLOAD_DIR = os.path.join(www_dir, UPLOAD_DIR)

try:
	os.makedirs(UPLOAD_DIR, exist_ok=True)
except OSError as e:
	print("<h2>Error: Cannot create folder " + html.escape(UPLOAD_DIR) + ": " + html.escape(str(e)) + "</h2>")
	exit()

# print("<hr>")

# Process upload using cgi.FieldStorage
try:
	form = cgi.FieldStorage()
		
	if 'file' not in form:
		raise Exception("No file field found in form")
		
	fileitem = form['file']
		
	if not fileitem.filename:
		raise Exception("No file selected")
		
	# Get filename
	filename = os.path.basename(fileitem.filename)
		
	# Save file
	saved_file_path = os.path.join(UPLOAD_DIR, filename)
		
	# Handle duplicate filenames
	base, extension = os.path.splitext(filename)
	counter = 1
	while os.path.exists(saved_file_path):
		saved_file_path = os.path.join(UPLOAD_DIR, base + "_" + str(counter) + extension)
		counter += 1
		
	# Write file
	with open(saved_file_path, "wb") as f:
		f.write(fileitem.file.read())
		
	final_filename = os.path.basename(saved_file_path)
		
	print("""
	<!DOCTYPE html>
	<html lang="en">
	<head>
		<meta charset="UTF-8">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		<title>Upload Success - Webserv</title>
		<style>
			@import url('https://fonts.googleapis.com/css2?family=Poppins:wght@400;600;700&display=swap');
			:root {
				--primary-color: #2c3e50;
				--secondary-color: #3498db;
				--accent-color: #e74c3c;
				--text-color: #f4f4f4;
				--bg-color: #ecf0f1;
				--card-bg: #ffffff;
				--shadow: 0 8px 16px rgba(0, 0, 0, 0.1);
			}
			body {
				font-family: 'Poppins', sans-serif;
				background-color: var(--bg-color);
				margin: 0;
				padding: 0;
				color: var(--primary-color);
				display: flex;
				justify-content: center;
				align-items: center;
				min-height: 100vh;
			}
			.container {
				background-color: var(--card-bg);
				padding: 3rem 4rem;
				border-radius: 16px;
				box-shadow: var(--shadow);
				width: 100%;
				max-width: 500px;
				transform: translateY(0);
				transition: transform 0.5s ease-in-out, box-shadow 0.5s ease;
				text-align: center;
			}
			.container:hover {
				transform: translateY(-8px);
				box-shadow: 0 12px 24px rgba(0, 0, 0, 0.15);
			}
			h1 {
				font-size: 3rem;
				font-weight: 700;
				color: var(--secondary-color);
				margin: 0 0 0.5rem;
				letter-spacing: -2px;
			}
			h2 {
				font-size: 1.8rem;
				color: var(--primary-color);
				margin: 0 0 2rem;
				font-weight: 600;
			}
			p {
				font-size: 1.1rem;
				line-height: 1.8;
				margin: 0 0 2.5rem;
			}
			.file-info {
				background: #f8f9fa;
				padding: 20px;
				border-radius: 8px;
				margin: 20px 0;
			}
			.back-link {
				text-align: center;
				margin-top: 2rem;
			}
			.back-link a {
				color: var(--secondary-color);
				text-decoration: none;
				font-weight: 600;
				transition: color 0.3s ease;
			}
			.back-link a:hover {
				color: var(--accent-color);
			}
		</style>
	</head>
	<body>
		<div class="container">
			<h1>✅</h1>
			<h2>File Uploaded Successfully!</h2>
			<div class="file-info">
				<p><strong>File name:</strong> """ + html.escape(final_filename) + """</p>
				<p><strong>Size:</strong> """ + str(os.path.getsize(saved_file_path)) + """ bytes (""" + str(round(os.path.getsize(saved_file_path) / 1024, 1)) + """ KB)</p>
			</div>
			<div class="back-link">
				<a href="/index.html">← Back to Homepage</a>
			</div>
		</div>
	</body>
	</html>
	""")
		
except Exception as e:
	print("""
	<!DOCTYPE html>
	<html lang="en">
	<head>
		<meta charset="UTF-8">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		<title>Upload Error - Webserv</title>
		<style>
			@import url('https://fonts.googleapis.com/css2?family=Poppins:wght@400;600;700&display=swap');
			:root {
				--primary-color: #2c3e50;
				--secondary-color: #3498db;
				--accent-color: #e74c3c;
				--text-color: #f4f4f4;
				--bg-color: #ecf0f1;
				--card-bg: #ffffff;
				--shadow: 0 8px 16px rgba(0, 0, 0, 0.1);
			}
			body {
				font-family: 'Poppins', sans-serif;
				background-color: var(--bg-color);
				margin: 0;
				padding: 0;
				color: var(--primary-color);
				display: flex;
				justify-content: center;
				align-items: center;
				min-height: 100vh;
			}
			.container {
				background-color: var(--card-bg);
				padding: 3rem 4rem;
				border-radius: 16px;
				box-shadow: var(--shadow);
				width: 100%;
				max-width: 500px;
				transform: translateY(0);
				transition: transform 0.5s ease-in-out, box-shadow 0.5s ease;
				text-align: center;
			}
			.container:hover {
				transform: translateY(-8px);
				box-shadow: 0 12px 24px rgba(0, 0, 0, 0.15);
			}
			h1 {
				font-size: 3rem;
				font-weight: 700;
				color: var(--accent-color);
				margin: 0 0 0.5rem;
				letter-spacing: -2px;
			}
			h2 {
				font-size: 1.8rem;
				color: var(--primary-color);
				margin: 0 0 2rem;
				font-weight: 600;
			}
			p {
				font-size: 1.1rem;
				line-height: 1.8;
				margin: 0 0 2.5rem;
			}
			.back-link {
				text-align: center;
				margin-top: 2rem;
			}
			.back-link a {
				color: var(--secondary-color);
				text-decoration: none;
				font-weight: 600;
				transition: color 0.3s ease;
			}
			.back-link a:hover {
				color: var(--accent-color);
			}
		</style>
	</head>
	<body>
		<div class="container">
			<h1>⚠️</h1>
			<h2>Upload Failed</h2>
			<p>Error: """ + html.escape(str(e)) + """</p>
			<div class="back-link">
				<a href="/upload.html">← Try Again</a>
			</div>
		</div>
	</body>
	</html>
	""")



# #!/usr/bin/env python3
# import os
# import html
# import sys

# print("Content-Type: text/html\n")

# # Debug info
# print("<h1>DEBUG INFO</h1>")
# print("<p>REQUEST_METHOD: " + os.environ.get('REQUEST_METHOD', 'NOT SET') + "</p>")
# print("<p>CONTENT_TYPE: " + os.environ.get('CONTENT_TYPE', 'NOT SET') + "</p>")
# print("<p>CONTENT_LENGTH: " + os.environ.get('CONTENT_LENGTH', 'NOT SET') + "</p>")

# # Read data in chunks to avoid deadlock
# content_length = int(os.environ.get("CONTENT_LENGTH", "0"))
# if content_length > 0:
# 	try:
# 		post_data = b""
# 		remaining = content_length
# 		chunk_size = 8192  # Same as webserv
		
# 		print("<p>Reading " + str(content_length) + " bytes in chunks of " + str(chunk_size) + "</p>")
		
# 		while remaining > 0:
# 			chunk = sys.stdin.buffer.read(min(remaining, chunk_size))
# 			if not chunk:
# 				print("<p>No more data, read " + str(len(post_data)) + " bytes</p>")
# 				break
			
# 			post_data += chunk
# 			remaining -= len(chunk)
# 			print("<p>Read " + str(len(chunk)) + " bytes, " + str(remaining) + " remaining</p>")
		
# 		print("<p>Total received: " + str(len(post_data)) + " bytes</p>")
		
# 	except Exception as e:
# 		print("<p>Error reading data: " + html.escape(str(e)) + "</p>")
# 		exit()
# else:
# 	print("<p>No data received</p>")
# 	exit()

# print("<hr>")

# # Setup upload directory
# UPLOAD_DIR = os.environ.get("UPLOAD_DIR", "upload")
# if not os.path.isabs(UPLOAD_DIR):
# 	script_dir = os.path.dirname(os.path.abspath(__file__))
# 	www_dir = os.path.dirname(script_dir)
# 	if UPLOAD_DIR.startswith('www/'):
# 		UPLOAD_DIR = UPLOAD_DIR[4:] 
# 	UPLOAD_DIR = os.path.join(www_dir, UPLOAD_DIR)

# try:
# 	os.makedirs(UPLOAD_DIR, exist_ok=True)
# except OSError as e:
# 	print("<h2>Error: Cannot create folder " + html.escape(UPLOAD_DIR) + ": " + html.escape(str(e)) + "</h2>")
# 	exit()

# # Parse multipart data manually (NO CGI MODULE!)
# def parse_multipart_form_data():
# 	content_type = os.environ.get("CONTENT_TYPE", "")
# 	if not content_type.startswith("multipart/form-data"):
# 		raise Exception("Invalid content type")
		
# 	boundary = content_type.split("boundary=")[1]
# 	raw_data = post_data
		
# 	boundary_bytes = ("--" + boundary).encode('utf-8')
# 	parts = raw_data.split(boundary_bytes)
		
# 	for part in parts:
# 		try:
# 			part_str = part.decode('utf-8', errors='ignore')
# 			if "name=\"file\"" in part_str and "filename=" in part_str:
# 				filename_line = [line for line in part_str.split("\r\n") if "filename=" in line][0]
# 				filename = filename_line.split("filename=")[1].strip('"')
				
# 				header_end = part.find(b"\r\n\r\n")
# 				if header_end == -1:
# 					continue
				
# 				file_data_start = header_end + 4
# 				file_data = part[file_data_start:]
				
# 				if file_data.endswith(b"\r\n"):
# 					file_data = file_data[:-2]
				
# 				return filename, file_data
# 		except Exception as e:
# 			continue
			
# 	raise Exception("No file found in form data")

# # Process upload
# try:
# 	filename, file_data = parse_multipart_form_data()
		
# 	if not filename:
# 		raise Exception("No file selected")
		
# 	# Save file
# 	saved_file_name = filename
# 	saved_file_path = os.path.join(UPLOAD_DIR, saved_file_name)

# 	# Handle duplicate filenames
# 	base, extension = os.path.splitext(saved_file_name)
# 	counter = 1
# 	while os.path.exists(saved_file_path):
# 		saved_file_path = os.path.join(UPLOAD_DIR, base + "_" + str(counter) + extension)
# 		counter += 1

# 	# Write file
# 	with open(saved_file_path, "wb") as f:
# 		f.write(file_data)

# 	final_filename = os.path.basename(saved_file_path)

# 	print("""
# 	<!DOCTYPE html>
# 	<html lang="en">
# 	<head>
# 		<meta charset="UTF-8">
# 		<meta name="viewport" content="width=device-width, initial-scale=1.0">
# 		<title>Upload Success - Webserv</title>
# 		<style>
# 			body { font-family: Arial, sans-serif; text-align: center; padding: 50px; }
# 			.container { max-width: 500px; margin: 0 auto; }
# 			h1 { color: #27ae60; }
# 			.file-info { background: #f8f9fa; padding: 20px; border-radius: 8px; margin: 20px 0; }
# 			a { color: #3498db; text-decoration: none; }
# 		</style>
# 	</head>
# 	<body>
# 		<div class="container">
# 			<h1>✅ File Uploaded Successfully!</h1>
# 			<div class="file-info">
# 				<p><strong>File name:</strong> """ + html.escape(final_filename) + """</p>
# 				<p><strong>Size:</strong> """ + str(len(file_data)) + """ bytes (""" + str(round(len(file_data) / 1024, 1)) + """ KB)</p>
# 			</div>
# 			<a href="/index.html">← Back to Homepage</a>
# 		</div>
# 	</body>
# 	</html>
# 	""")
		
# except Exception as e:
# 	print("""
# 	<!DOCTYPE html>
# 	<html lang="en">
# 	<head>
# 		<meta charset="UTF-8">
# 		<meta name="viewport" content="width=device-width, initial-scale=1.0">
# 		<title>Upload Error - Webserv</title>
# 		<style>
# 			body { font-family: Arial, sans-serif; text-align: center; padding: 50px; }
# 			.container { max-width: 500px; margin: 0 auto; }
# 			h1 { color: #e74c3c; }
# 			a { color: #3498db; text-decoration: none; }
# 		</style>
# 	</head>
# 	<body>
# 		<div class="container">
# 			<h1>⚠️ Upload Failed</h1>
# 			<p>Error: """ + html.escape(str(e)) + """</p>
# 			<a href="/upload.html">← Try Again</a>
# 		</div>
# 	</body>
# 	</html>
# 	""")