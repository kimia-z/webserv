#!/usr/bin/env python3
import cgi
import os
import html

print("Content-Type: text/html\n")

# dir where the files will be uploaded to
UPLOAD_DIR = os.environ.get("UPLOAD_DIR", "/tmp/uploads")

# pull the limit data from env
try:
    MAX_FILE_SIZE = int(os.environ.get("MAX_FILE_SIZE", "1048576"))
except ValueError:
    MAX_FILE_SIZE = 1048576  # or set the default of 1 MB

# making sure the upload exists
try:
    os.makedirs(UPLOAD_DIR, exist_ok=True)
except OSError as e:
    print(f"<h2>Error: Folder {html.escape(UPLOAD_DIR)} cannot be created: {html.escape(str(e))}</h2>")
    exit()

form = cgi.FieldStorage()
file_item = form["file"]

if file_item.filename:
    file_data = file_item.file.read()

    if len(file_data) > MAX_FILE_SIZE:
        print(f"""
        <html>
        <head><meta charset="UTF-8"><title>Error</title></head>
        <body>
          <h2>too big file</h2>
          <p>max size is {MAX_FILE_SIZE} bytes.</p>
        </body>
        </html>
        """)
    else:
        # save the file
        save_name = os.path.basename(file_item.filename)
        save_path = os.path.join(UPLOAD_DIR, save_name)

        with open(save_path, "wb") as f:
            f.write(file_data)

        print(f"""
        <html>
        <head><meta charset="UTF-8"><title>Success</title></head>
        <body>
          <h2>File has been saved</h2>
          <p>File name: {html.escape(save_name)}</p>
          <p>Size: {len(file_data)} bytes</p>
        </body>
        </html>
        """)
else:
    print("""
    <html>
    <head><meta charset="UTF-8"><title>Error</title></head>
    <body>
      <h2>no file attached</h2>
    </body>
    </html>
    """)
