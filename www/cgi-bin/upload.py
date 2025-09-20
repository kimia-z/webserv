#!/usr/bin/env python3
import cgi
import os
import html

print("Content-Type: text/html\n")

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

try:
	form = cgi.FieldStorage()
		
	if 'file' not in form:
		raise Exception("No file field found in form")
		
	fileitem = form['file']
		
	if not fileitem.filename:
		raise Exception("No file selected")
		
	filename = os.path.basename(fileitem.filename)
		
	saved_file_path = os.path.join(UPLOAD_DIR, filename)
		
	base, extension = os.path.splitext(filename)
	counter = 1
	while os.path.exists(saved_file_path):
		saved_file_path = os.path.join(UPLOAD_DIR, base + "_" + str(counter) + extension)
		counter += 1
		
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