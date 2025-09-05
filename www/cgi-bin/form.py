#!/usr/bin/env python3

import cgi
import html
import os

print("Content-Type: text/html\n")

current_dir = os.path.dirname(os.path.abspath(__file__))
DATA_FILE = os.path.join(os.path.dirname(current_dir), "form_table.txt")

form = cgi.FieldStorage()
name = html.escape(form.getvalue("name", ""))
phone = html.escape(form.getvalue("phone", ""))
email = html.escape(form.getvalue("email", ""))

try:
	os.makedirs(os.path.dirname(DATA_FILE), exist_ok=True)
except OSError as e:
	print(f"""
	<html>
	<head><meta charset="UTF-8"><title>Error</title></head>
	<body><h2>Error: Could not create directory: {html.escape(str(e))}</h2></body>
	</html>
	""")
	exit()


try:
	with open(DATA_FILE, "a", encoding="utf-8") as file:
		file.write(f"{name} | {phone} | {email}\n")
except OSError as e:
	print(f"""
	<html>
	<head><meta charset="UTF-8"><title>Error</title></head>
	<body><h2>Error: Could not write to file: {html.escape(str(e))}</h2></body>
	</html>
	""")
	exit()


print(f"""
<!DOCTYPE html>
<html lang="en">
<head><meta charset="UTF-8"><title>Thank you!</title></head>
<body>
<h2>Thank you {name}, for submitting the form!</h2>
<p>We will contact you at {email} or {phone}.</p>
<a href="/index.html">homepage</a>
</body>
</html>
""")