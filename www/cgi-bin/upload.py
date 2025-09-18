#!/usr/bin/env python3
import os
import html
import sys
import io

print("Content-Type: text/html\n")
print("<h1>Debug Info</h1>")
print("<h1>DEBUG INFO</h1>")
print(f"<p>REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'NOT SET')}</p>")
print(f"<p>CONTENT_TYPE: {os.environ.get('CONTENT_TYPE', 'NOT SET')}</p>")
print(f"<p>CONTENT_LENGTH: {os.environ.get('CONTENT_LENGTH', 'NOT SET')}</p>")
print(f"<p>QUERY_STRING: {os.environ.get('QUERY_STRING', 'NOT SET')}</p>")


content_length = int(os.environ.get("CONTENT_LENGTH", "0"))
if content_length > 0:
	try:
		post_data = sys.stdin.read(content_length)
		print(f"<p>Received {len(post_data)} bytes of POST data</p>")
		print(f"<p>First 200 chars: {html.escape(post_data[:200])}</p>")
	except Exception as e:
		print(f"<p>Error reading POST data: {html.escape(str(e))}</p>")
else:
	print("<p>No POST data received</p>")

print("<hr>")

UPLOAD_DIR = os.environ.get("UPLOAD_DIR", "upload")

if not os.path.isabs(UPLOAD_DIR):
	script_dir = os.path.dirname(os.path.abspath(__file__))
	www_dir = os.path.dirname(script_dir)
	if UPLOAD_DIR.startswith('www/'):
		UPLOAD_DIR = UPLOAD_DIR[4:] 
	UPLOAD_DIR = os.path.join(www_dir, UPLOAD_DIR)

try:
	MAX_FILE_SIZE = int(os.environ.get("MAX_FILE_SIZE", "1048576"))
except ValueError:
	MAX_FILE_SIZE = 1048576

try:
	os.makedirs(UPLOAD_DIR, exist_ok=True)
except OSError as e:
	print(f"<h2>Error: Folder {html.escape(UPLOAD_DIR)} cannot be created: {html.escape(str(e))}</h2>")
	exit()

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
		try:
			part_str = part.decode('utf-8', errors='ignore')
			if "name=\"file\"" in part_str and "filename=" in part_str:
				# Extract filename
				filename_line = [line for line in part_str.split("\r\n") if "filename=" in line][0]
				filename = filename_line.split("filename=")[1].strip('"')
				
				# Extract file data (skip headers)
				header_end = part.find(b"\r\n\r\n")
				if header_end == -1:
					continue  # Skip this part if no header separator
				
				file_data_start = header_end + 4
				file_data = part[file_data_start:]
				
				# Remove trailing boundary
				if file_data.endswith(b"\r\n"):
					file_data = file_data[:-2]
				
				return filename, file_data
		except Exception as e:
			continue  # Skip this part if parsing fails
		
	raise Exception("No file found in form data")

# def parse_multipart_form_data():
# 	content_type = os.environ.get("CONTENT_TYPE", "")
# 	if not content_type.startswith("multipart/form-data"):
# 		raise Exception("Invalid content type")
	
# 	boundary = content_type.split("boundary=")[1]
# 	content_length = int(os.environ.get("CONTENT_LENGTH", "0"))
	
# 	if content_length == 0:
# 		raise Exception("No content")
	
# 	# Read the raw input as binary
# 	raw_data = sys.stdin.buffer.read(content_length)
	
# 	# Find the file part
# 	boundary_bytes = ("--" + boundary).encode('utf-8')
# 	parts = raw_data.split(boundary_bytes)
# 	for part in parts:
# 		part_str = part.decode('utf-8', errors='ignore')
# 		if "name=\"file\"" in part_str and "filename=" in part_str:
# 			# Extract filename
# 			filename_line = [line for line in part_str.split("\r\n") if "filename=" in line][0]
# 			filename = filename_line.split("filename=")[1].strip('"')
			
# 			# Extract file data (skip headers)
# 			file_data_start = part.find(b"\r\n\r\n") + 4
# 			file_data = part[file_data_start:]
			
# 			# Remove trailing boundary
# 			if file_data.endswith(b"\r\n"):
# 				file_data = file_data[:-2]
			
# 			return filename, file_data
	
# 	raise Exception("No file found in form data")

try:
	filename, file_data = parse_multipart_form_data()
	
	if filename:
		if len(file_data) > MAX_FILE_SIZE:
			print(f"""
		<!DOCTYPE html>
		<html lang="en">
		<head>
			<meta charset="UTF-8">
			<meta name="viewport" content="width=device-width, initial-scale=1.0">
			<title>File Too Large - Webserv</title>
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
					color: var(--accent-color);
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
				<h1>📏</h1>
				<h2>File Too Large</h2>
				<p>Maximum file size is {MAX_FILE_SIZE // 1024 // 1024}MB ({MAX_FILE_SIZE} bytes).</p>
				<div class="back-link">
					<a href="/upload.html">← Try Again</a>
				</div>
			</div>
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
			<!DOCTYPE html>
			<html lang="en">
			<head>
				<meta charset="UTF-8">
				<meta name="viewport" content="width=device-width, initial-scale=1.0">
				<title>Upload Success - Webserv</title>
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
						color: var(--secondary-color);
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
						margin: 0 0 1rem;
					}}

					.file-info {{
						background-color: var(--bg-color);
						padding: 1rem;
						border-radius: 8px;
						margin: 1rem 0;
						font-family: monospace;
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
					<h1>✅</h1>
					<h2>File Uploaded Successfully!</h2>
					<div class="file-info">
						<p><strong>File name:</strong> {html.escape(final_filename)}</p>
						<p><strong>Size:</strong> {len(file_data)} bytes ({(len(file_data) / 1024):.1f} KB)</p>
					</div>
					<div class="back-link">
						<a href="/index.html">← Back to Homepage</a>
					</div>
				</div>
			</body>
			</html>
			""")
	else:
		print("""
		<!DOCTYPE html>
		<html lang="en">
		<head>
			<meta charset="UTF-8">
			<meta name="viewport" content="width=device-width, initial-scale=1.0">
			<title>No File Selected - Webserv</title>
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
					color: var(--accent-color);
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
				<h1>📁</h1>
				<h2>No File Selected</h2>
				<p>Please select a file to upload before submitting the form.</p>
				<div class="back-link">
					<a href="/upload.html">← Try Again</a>
				</div>
			</div>
		</body>
		</html>
		""")
		
except Exception as e:
	print(f"""
	<!DOCTYPE html>
	<html lang="en">
	<head>
		<meta charset="UTF-8">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		<title>Upload Error - Webserv</title>
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
				color: var(--accent-color);
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
			<h1>⚠️</h1>
			<h2>Upload Failed</h2>
			<p>Error: {html.escape(str(e))}</p>
			<div class="back-link">
				<a href="/upload.html">← Try Again</a>
			</div>
		</div>
	</body>
	</html>
	""")