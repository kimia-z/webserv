#!/usr/bin/env python3

import os
import html
import urllib.parse

print("Content-Type: text/html")

DELETE_TARGET = os.environ.get("DELETE_TARGET", "")
UPLOAD_DIR = os.environ.get("UPLOAD_DIR", "upload")

if not os.path.isabs(UPLOAD_DIR):
	script_dir = os.path.dirname(os.path.abspath(__file__))
	www_dir = os.path.dirname(script_dir)
	if UPLOAD_DIR.startswith('www/'):
		UPLOAD_DIR = UPLOAD_DIR[4:]
	UPLOAD_DIR = os.path.join(www_dir, UPLOAD_DIR)

try:
	if not DELETE_TARGET:
		raise Exception("No target specified for deletion")
	#get the file name from url encoding
	decoded_target = urllib.parse.unquote(DELETE_TARGET)

	#get full path to the file
	target_path = os.path.join(UPLOAD_DIR, decoded_target)

	#check if the file exists
	if not os.path.isfile(target_path):
		raise Exception(f"'{decoded_target} does not exist")
	
	real_target_path = os.path.realpath(target_path)
	real_upload_dir = os.path.realpath(UPLOAD_DIR)
	if not real_target_path.startswith(real_upload_dir):
		raise Exception("Invalid target path (outside of upload directory)")
	
	os.remove(target_path)


	print(f"""
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>File Deleted - Webserv</title>
        <link rel="icon" type="image/x-icon" href="images/favicon.ico">
        <style>
            @import url('https://fonts.googleapis.com/css2?family=Poppins:wght@400;600;700&display=swap');
            
            :root {{
                --primary-color: #2c3e50;
                --secondary-color: #3498db;
                --accent-color: #e74c3c;
                --success-color: #27ae60;
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
                color: var(--success-color);
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
                border-left: 4px solid var(--success-color);
            }}

            .actions {{
                display: flex;
                gap: 1rem;
                justify-content: center;
                margin-top: 2rem;
                flex-wrap: wrap;
            }}

            .actions a {{
                color: var(--secondary-color);
                text-decoration: none;
                font-weight: 600;
                padding: 0.5rem 1rem;
                border: 2px solid var(--secondary-color);
                border-radius: 8px;
                transition: all 0.3s ease;
            }}

            .actions a:hover {{
                background-color: var(--secondary-color);
                color: white;
            }}

            .actions a.danger {{
                color: var(--accent-color);
                border-color: var(--accent-color);
            }}

            .actions a.danger:hover {{
                background-color: var(--accent-color);
                color: white;
            }}
        </style>
    </head>
    <body>
        <div class="container">
            <h1>🗑️</h1>
            <h2>File Deleted Successfully!</h2>
            <div class="file-info">
                <p><strong>Deleted file:</strong> {html.escape(decoded_target)}</p>
                <p><strong>Location:</strong> {html.escape(UPLOAD_DIR)}</p>
            </div>
            <div class="actions">
                <a href="/index.html">← Back to Homepage</a>
                <a href="/upload.html">📁 Upload More Files</a>
                <a href="/upload/" class="danger">📋 View Upload Directory</a>
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
        <title>Delete Failed - Webserv</title>
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

            .error-info {{
                background-color: #ffe6e6;
                padding: 1rem;
                border-radius: 8px;
                margin: 1rem 0;
                font-family: monospace;
                border-left: 4px solid var(--accent-color);
                color: #8b0000;
            }}

            .actions {{
                display: flex;
                gap: 1rem;
                justify-content: center;
                margin-top: 2rem;
                flex-wrap: wrap;
            }}

            .actions a {{
                color: var(--secondary-color);
                text-decoration: none;
                font-weight: 600;
                padding: 0.5rem 1rem;
                border: 2px solid var(--secondary-color);
                border-radius: 8px;
                transition: all 0.3s ease;
            }}
            .actions a:hover {{
                background-color: var(--secondary-color);
                color: white;
            }}
        </style>
    </head>
    <body>
        <div class="container">
            <h1>⚠️</h1>
            <h2>Delete Failed</h2>
            <div class="error-info">
                <p><strong>Error:</strong> {html.escape(str(e))}</p>
                <p><strong>Target:</strong> {html.escape(DELETE_TARGET)}</p>
            </div>
            <div class="actions">
                <a href="/index.html">← Back to Homepage</a>
                <a href="/upload.html">📁 Upload Files</a>
                <a href="/upload/">📋 View Upload Directory</a>
            </div>
        </div>
    </body>
    </html>
    """)