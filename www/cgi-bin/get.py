#!/usr/bin/env python3
import os
import html
import urllib.parse

print("Content-Type: text/html\n")

query_string = os.environ.get("QUERY_STRING", "")
search_term = ""

if query_string:
	params = urllib.parse.parse_qs(query_string)
	if "search_term" in params:
		search_term = params["search_term"][0].strip()

current_dir = os.path.dirname(os.path.abspath(__file__))
DATA_FILE = os.path.join(os.path.dirname(current_dir), "form_table.txt")

def load_user_data():
	users = []
	try:
		if os.path.exists(DATA_FILE):
			with open(DATA_FILE, 'r', encoding='utf-8') as f:
				for line in f:
					line = line.strip()
					if line and ' | ' in line:
						parts = line.split(' | ')
						if len(parts) >= 3:
							users.append({
								'name': parts[0].strip(),
								'phone': parts[1].strip(),
								'email': parts[2].strip()
							})
		return users
	except Exception as e:
		print(f"Error loading data: {e}")
		return []

def search_users(users, search_term):
	if not search_term:
		return []
		
	search_term_lower = search_term.lower().strip()
	found_users = []
		
	for user in users:
		phone_match = user.get('phone', '').lower().strip() == search_term_lower
		email_match = user.get('email', '').lower().strip() == search_term_lower
		
		if phone_match or email_match:
			found_users.append(user)
		
	return found_users

def format_user_data(user):
	return f"""
	<div class="user-card">
		<div class="user-header">
			<h3>👤 {html.escape(user.get('name', 'N/A'))}</h3>
		</div>
		<div class="user-details">
			<div class="detail-item">
				<span class="detail-label">📧 Email:</span>
				<span class="detail-value">{html.escape(user.get('email', 'N/A'))}</span>
			</div>
			<div class="detail-item">
				<span class="detail-label">📱 Phone:</span>
				<span class="detail-value">{html.escape(user.get('phone', 'N/A'))}</span>
			</div>
		</div>
	</div>
	"""

users = load_user_data()

found_users = search_users(users, search_term)

print(f"""
<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title>Search Results - Webserv</title>
	<link rel="icon" type="image/x-icon" href="favicon.png">
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
			padding: 20px;
			color: var(--primary-color);
			min-height: 100vh;
		}}

		.container {{
			max-width: 1000px;
			margin: 0 auto;
			background-color: var(--card-bg);
			padding: 3rem 4rem;
			border-radius: 16px;
			box-shadow: var(--shadow);
			transform: translateY(0);
			transition: transform 0.5s ease-in-out, box-shadow 0.5s ease;
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
			text-align: center;
		}}

		h2 {{
			font-size: 1.8rem;
			color: var(--primary-color);
			margin: 0 0 2rem;
			font-weight: 600;
			text-align: center;
		}}

		.search-info {{
			background-color: var(--bg-color);
			padding: 1.5rem;
			border-radius: 12px;
			margin: 2rem 0;
			border-left: 4px solid var(--secondary-color);
			text-align: center;
		}}

		.user-card {{
			background-color: var(--bg-color);
			padding: 2rem;
			border-radius: 12px;
			margin: 1.5rem 0;
			border-left: 4px solid var(--success-color);
			box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
		}}

		.user-header h3 {{
			margin: 0 0 1.5rem;
			color: var(--success-color);
			font-size: 1.5rem;
		}}

		.user-details {{
			display: grid;
			gap: 1rem;
		}}

		.detail-item {{
			display: flex;
			align-items: center;
			padding: 0.5rem 0;
			border-bottom: 1px solid #ddd;
		}}

		.detail-item:last-child {{
			border-bottom: none;
		}}

		.detail-label {{
			font-weight: 600;
			color: var(--primary-color);
			min-width: 120px;
			margin-right: 1rem;
		}}

		.detail-value {{
			color: var(--secondary-color);
			flex: 1;
			word-break: break-word;
		}}

		.no-results {{
			text-align: center;
			padding: 3rem;
			color: var(--accent-color);
			font-size: 1.2rem;
		}}

		.no-results h3 {{
			font-size: 2rem;
			margin-bottom: 1rem;
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
		<h1>🔍</h1>
		<h2>Search Results</h2>
		
		<div class="search-info">
			<strong>Search term:</strong> "{html.escape(search_term)}"<br>
			<strong>Results found:</strong> {len(found_users)} user(s)
		</div>
		
		{''.join([format_user_data(user) for user in found_users]) if found_users else '''
		<div class="no-results">
			<h3>😔</h3>
			<h3>No Users Found</h3>
			<p>No users match your search criteria. Try searching with a different phone number or email address, or add new users first.</p>
		</div>
		'''}
		<div class="back-link">
			<a href="/index.html">← Back to Homepage</a>
		</div>

	</div>
</body>
</html>
""")