#!/usr/bin/env python3
import cgi
import html
import os

current_dir = os.path.dirname(os.path.abspath(__file__))
FILE_PATH = os.path.join(current_dir, "../form_table.txt")

print("Content-Type: text/html\n")

form = cgi.FieldStorage()
input_value = form.getvalue("contact", "").strip()

def remove_entry(contact):
	if not os.path.exists(FILE_PATH):
		return False, "data file does not exist"

	removed = False
	with open(FILE_PATH, "r", encoding="utf-8") as file:
		lines = file.readlines()

	with open(FILE_PATH, "w", encoding="utf-8") as file:
		for line in lines:
			components = line.strip().split(' | ')
			if len(components) == 3:
				name, phone, email = components
				if contact == phone or contact == email:
					removed = True
					continue
			file.write(line)

	if os.stat(FILE_PATH).st_size == 0:
		os.remove(FILE_PATH)

	return removed, "entry was removed" if removed else "contact was not found"

escaped_contact = html.escape(input_value)

if escaped_contact:
	success, message = remove_entry(escaped_contact)
else:
	message = "no contact submitted"
	success = False

if success:
	icon = "✅"
	title = "Contact Removed Successfully"
	color = "var(--secondary-color)"
else:
	icon = "❌"
	title = "Contact Removal Failed"
	color = "var(--accent-color)"

print(f"""
<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title>Contact Removal - Webserv</title>
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
		<p>{html.escape(message)}</p>
		<div class="back-link">
			<a href="/index.html">← Back to Homepage</a>
		</div>
	</div>
</body>
</html>
""")