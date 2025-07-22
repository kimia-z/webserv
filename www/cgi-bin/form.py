#!/usr/bin/env python3

import cgi
import html
import os

print("Content-Type: text/html\n")

form = cgi.FieldStorage()
name = html.escape(form.getvalue("name", ""))
phone = form.escape(form.getvalue("phone", ""))
email = html.escape(form.getvalue("email", ""))

with open("/tmp/form_data.txt", "a", encoding="utf-8") as file:
	file.write(f"{name} | {phone} | {email}\n") 

print(f"""
<!DOCTYPE html>
<html lang="en">
<head><meta charset="UTF-8"><title>Thank you!</title></head>
<body>
<h2>Thank you {name}, for submitting the form!</h2>
<p>We will contact you at {email} or {phone}.</p>
</body>
</html>
""")