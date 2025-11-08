import subprocess

Import("env")

# Get the current git commit hash
try:
    commit_hash = subprocess.check_output(['git', 'rev-parse', 'HEAD']).strip().decode('utf-8')
except (subprocess.CalledProcessError, FileNotFoundError):
    commit_hash = "unknown"

# Define the content for the version.h file
header_content = f'#pragma once\nconst char GIT_COMMIT_HASH[] = "{commit_hash}";\n'

# Write the content to version.h
with open('src/version.h', 'w') as header_file:
    header_file.write(header_content)
