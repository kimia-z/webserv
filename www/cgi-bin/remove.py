#!/usr/bin/env python3
import cgi
import html
import os

FILE_PATH = "/tmp/form_data.txt"

print("Content-Type: text/html\n")

form = cgi.FieldStorage()
input_value = form.getvalue("contact", "").strip()

def remove_entry(contact):
	if not os.path.exists(FILE_PATH):
		return False, "The data file does not exist"

	removed = False
	with open(FILE_PATH, "r", encoding="utf-8") as file:
		lines = file.readlines()

	with open(FILE_PATH, "w", encoding="utf-8") as file:
		for line in lines:
			if contact in line:
				removed = True
				continue  #skipping this line
			file.write(line)

	# if the file is empty after removal, it will be deleted
	if os.stat(FILE_PATH).st_size == 0:
		os.remove(FILE_PATH)

	return removed, "entry was removed" if removed else "contact was not found"

escaped_contact = html.escape(input_value)

if escaped_contact:
	success, message = remove_entry(escaped_contact)
else:
	success = False
	message = "no contact submitted"

print(f"""
<!DOCTYPE html>
<html lang="pl">
<head><meta charset="UTF-8"><title>contact details removal</title></head>
<body>
<h2>you successfully deleted your contact details</h2>
<p>{html.escape(message)}</p>
<a href="/index.html">homepage</a>
</body>
</html>
""")