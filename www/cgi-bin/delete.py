#!/usr/bin/env python3
import sys
import html
import os
import urllib.parse

print("Content-Type: text/html\n")

UPLOAD_DIR = os.environ.get("UPLOAD_DIR", "upload")

# Resolve absolute path for UPLOAD_DIR
if not os.path.isabs(UPLOAD_DIR):
	script_dir = os.path.dirname(os.path.abspath(__file__))
	www_dir = os.path.dirname(script_dir)
	if UPLOAD_DIR.startswith('www/'):
		UPLOAD_DIR = UPLOAD_DIR[4:]
	UPLOAD_DIR = os.path.join(www_dir, UPLOAD_DIR)


method = os.environ.get("REQUEST_METHOD", "GET").upper()

if method == "DELETE":
	# parse query string for DELETE requests
	query_string = os.environ.get("QUERY_STRING", "")
	params = urllib.parse.parse_qs(query_string)
	DELETE_TARGET = params.get("file", [""])[0]
else:
	# Use CGI form for GET or POST
	if method == "POST":
		content_length = int(os.environ.get("CONTENT_LENGTH", 0))
		if content_length > 0:
			post_data = sys.stdin.read(content_length)
			form_data = urllib.parse.parse_qs(post_data)
			DELETE_TARGET = form_data.get("DELETE_TARGET", [""])[0]
		else:
			DELETE_TARGET = ""

decoded_target = urllib.parse.unquote(DELETE_TARGET)

def delete_file(filename):
	try:
		target_path = os.path.join(UPLOAD_DIR, filename)

		debug_info = f"""
        <!-- DEBUG INFO:
        UPLOAD_DIR: {UPLOAD_DIR}
        filename: {filename}
        target_path: {target_path}
        target_path exists: {os.path.isfile(target_path)}
        target_path realpath: {os.path.realpath(target_path) if os.path.exists(target_path) else 'N/A'}
        UPLOAD_DIR realpath: {os.path.realpath(UPLOAD_DIR)}
        Current working directory: {os.getcwd()}
        Script location: {os.path.abspath(__file__)}
        -->
        """

		print(debug_info)

		if not os.path.isfile(target_path):
			return False, f"File '{filename}' does not exist"

		real_target_path = os.path.realpath(target_path)
		real_upload_dir = os.path.realpath(UPLOAD_DIR)
		if not real_target_path.startswith(real_upload_dir):
			return False, "Invalid target path (outside of upload directory)"

		os.remove(target_path)
		return True, f"Deleted file: {filename}"
	except Exception as e:
		return False, str(e)

success, message = delete_file(decoded_target)

# Choose styling based on success/failure
if success:
	icon = "✅"
	title = "File Deleted Successfully"
	color = "var(--secondary-color)"
else:
	icon = "⚠️"
	title = "Delete Failed"
	color = "var(--accent-color)"

escaped_message = html.escape(message)
escaped_file = html.escape(decoded_target)

# HTML output in the same style as your other scripts
print(f"""
<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title>{title} - Webserv</title>
	<link rel="icon" type="image/x-icon" href="images/favicon.ico">
	<style>
		@import url('https://fonts.googleapis.com/css2?family=Poppins:wght@400;600;700&display=swap');

		:root {{
			--primary-color: #2c3e50;
			--secondary-color: #3498db;
			--accent-color: #e74c3c;
			--text-color: #f4f4f4;
			--bg-color: #ecf0f1;
			--card-bg: #ffffff;
			--shadow: 0 8px 16px rgba(0, 0, 0, 0.1);
		}}

		body {{
			font-family: 'Poppins', sans-serif;
			background-color: var(--bg-color);
			margin: 0;
			padding: 0;
			color: var(--primary-color);
			display: flex;
			justify-content: center;
			align-items: center;
			min-height: 100vh;
		}}

		.container {{
			background-color: var(--card-bg);
			padding: 3rem 4rem;
			border-radius: 16px;
			box-shadow: var(--shadow);
			width: 100%;
			max-width: 500px;
			transform: translateY(0);
			transition: transform 0.5s ease-in-out, box-shadow 0.5s ease;
			text-align: center;
		}}

		.container:hover {{
			transform: translateY(-8px);
			box-shadow: 0 12px 24px rgba(0, 0, 0, 0.15);
		}}

		h1 {{
			font-size: 3rem;
			font-weight: 700;
			color: {color};
			margin: 0 0 0.5rem;
			letter-spacing: -2px;
		}}

		h2 {{
			font-size: 1.8rem;
			color: var(--primary-color);
			margin: 0 0 2rem;
			font-weight: 600;
		}}

		p {{
			font-size: 1.1rem;
			line-height: 1.8;
			margin: 0 0 2.5rem;
		}}

		.back-link {{
			text-align: center;
			margin-top: 2rem;
		}}

		.back-link a {{
			color: var(--secondary-color);
			text-decoration: none;
			font-weight: 600;
			transition: color 0.3s ease;
		}}

		.back-link a:hover {{
			color: var(--accent-color);
		}}
	</style>
</head>
<body>
	<div class="container">
		<h1>{icon}</h1>
		<h2>{title}</h2>
		<p>{escaped_message}</p>
		<div class="back-link">
			<a href="/index.html">← Back to Homepage</a>
		</div>
	</div>
</body>
</html>
""")


