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
	<!DOCTYPE html>
	<html lang="en">
	<head>
		<meta charset="UTF-8">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		<title>Error - Webserv</title>
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
			<h2>Error: Could not create directory</h2>
			<p>{html.escape(str(e))}</p>
			<div class="back-link">
				<a href="/form.html">← Go back to form</a>
			</div>
		</div>
	</body>
	</html>
	""")
	exit()


try:
	with open(DATA_FILE, "a", encoding="utf-8") as file:
		file.write(f"{name} | {phone} | {email}\n")
except OSError as e:
	print(f"""
	<!DOCTYPE html>
	<html lang="en">
	<head>
		<meta charset="UTF-8">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		<title>Error - Webserv</title>
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
			<h2>Error: Could not write to file</h2>
			<p>{html.escape(str(e))}</p>
			<div class="back-link">
				<a href="/form.html">← Go back to form</a>
			</div>
		</div>
	</body>
	</html>
	""")
	exit()


print(f"""
<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title>Thank You - Webserv</title>
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
		<h1>✅</h1>
		<h2>Thank you {name}, for submitting the form!</h2>
		<p>We will contact you at {email} or {phone}.</p>
		<div class="back-link">
			<a href="/index.html">← Back to Homepage</a>
		</div>
	</div>
</body>
</html>
""")