#!/usr/bin/env python
#
# E-Mail post-processing script for NZBGet
#
# Copyright (C) 2013-2014 Andrey Prygunkov <hugbug@users.sourceforge.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
# $Revision: 1107 $
# $Date: 2014-08-27 18:27:40 +0200 (Wed, 27 Aug 2014) $
#


##############################################################################
### NZBGET POST-PROCESSING SCRIPT                                          ###

# Send E-Mail notification.
#
# This script sends E-Mail notification when the job is done.
#
# NOTE: This script requires Python to be installed on your system.

##############################################################################
### OPTIONS                                                       ###

# Email address you want this email to be sent from. 
#From="NZBGet" <myaccount@gmail.com>

# Email address you want this email to be sent to. 
#To=myaccount@gmail.com

# SMTP server host.
#Server=smtp.gmail.com

# SMTP server port (1-65535).
#Port=25

# Secure communication using TLS/SSL (yes, no).
#Encryption=yes

# SMTP server user name, if required.
#Username=myaccount

# SMTP server password, if required.
#Password=mypass

# Append statistics to the message (yes, no).
#Statistics=yes

# Append list of files to the message (yes, no).
#
# Add the list of downloaded files (the content of destination directory).
#FileList=yes

# Append broken-log to the message (yes, no).
#
# Add the content of file _brokenlog.txt. This file contains the list of damaged
# files and the result of par-check/repair. For successful downloads the broken-log
# is usually deleted by cleanup-script and therefore is not sent.
#BrokenLog=yes

# Append post-processing log to the message (Always, Never, OnFailure).
#
# Add the post-processing log of active job.
#PostProcessLog=OnFailure

### NZBGET POST-PROCESSING SCRIPT                                          ###
##############################################################################


import os
import sys
import datetime
import smtplib
from email.mime.text import MIMEText
try:
	from xmlrpclib import ServerProxy # python 2
except ImportError:
	from xmlrpc.client import ServerProxy # python 3

# Exit codes used by NZBGet
POSTPROCESS_SUCCESS=93
POSTPROCESS_ERROR=94

# Check if the script is called from nzbget 11.0 or later
if not 'NZBPP_TOTALSTATUS' in os.environ:
	print('*** NZBGet post-processing script ***')
	print('This script is supposed to be called from nzbget (13.0 or later).')
	sys.exit(POSTPROCESS_ERROR)

print('[DETAIL] Script successfully started')
sys.stdout.flush()

required_options = ('NZBPO_FROM', 'NZBPO_TO', 'NZBPO_SERVER', 'NZBPO_PORT', 'NZBPO_ENCRYPTION', 'NZBPO_USERNAME', 'NZBPO_PASSWORD')
for	optname in required_options:
	if (not optname in os.environ):
		print('[ERROR] Option %s is missing in configuration file. Please check script settings' % optname[6:])
		sys.exit(POSTPROCESS_ERROR)

status = os.environ['NZBPP_STATUS']
total_status = os.environ['NZBPP_TOTALSTATUS']

# If any script fails the status of the item in the history is "WARNING/SCRIPT".
# This status however is not passed to pp-scripts in the env var "NZBPP_STATUS"
# because most scripts are independent of each other and should work even
# if a previous script has failed. But not in the case of E-Mail script,
# which should take the status of the previous scripts into account as well.
if total_status == 'SUCCESS' and os.environ['NZBPP_SCRIPTSTATUS'] == 'FAILURE':
	total_status = 'WARNING'
	status = 'WARNING/SCRIPT'
		
success = total_status == 'SUCCESS'
if success:
	subject = 'Success for "%s"' % (os.environ['NZBPP_NZBNAME'])
	text = 'Download of "%s" has successfully completed.' % (os.environ['NZBPP_NZBNAME'])
else:
	subject = 'Failure for "%s"' % (os.environ['NZBPP_NZBNAME'])
	text = 'Download of "%s" has failed.' % (os.environ['NZBPP_NZBNAME'])

text += '\nStatus: %s' % status

if os.environ.get('NZBPO_STATISTICS') == 'yes' or \
	os.environ.get('NZBPO_POSTPROCESSLOG') == 'Always' or \
	(os.environ.get('NZBPO_POSTPROCESSLOG') == 'OnFailure' and not success):
	# To get statistics or the post-processing log we connect to NZBGet via XML-RPC.
	# For more info visit http://nzbget.net/RPC_API_reference
	# First we need to know connection info: host, port and password of NZBGet server.
	# NZBGet passes all configuration options to post-processing script as
	# environment variables.
	host = os.environ['NZBOP_CONTROLIP'];
	port = os.environ['NZBOP_CONTROLPORT'];
	username = os.environ['NZBOP_CONTROLUSERNAME'];
	password = os.environ['NZBOP_CONTROLPASSWORD'];
	
	if host == '0.0.0.0': host = '127.0.0.1'
	
	# Build an URL for XML-RPC requests
	rpcUrl = 'http://%s:%s@%s:%s/xmlrpc' % (username, password, host, port);
	
	# Create remote server object
	server = ServerProxy(rpcUrl)

if os.environ.get('NZBPO_STATISTICS') == 'yes':
	# Find correct nzb in method listgroups 
	groups = server.listgroups(0)
	nzbID = int(os.environ['NZBPP_NZBID'])
	for nzbGroup in groups:
		if nzbGroup['NZBID'] == nzbID:
			break

	text += '\n\nStatistics:';

	# add download size
	DownloadedSize = float(nzbGroup['DownloadedSizeMB'])
	unit = ' MB'
	if DownloadedSize > 1024:
		DownloadedSize = DownloadedSize / 1024 # GB
		unit = ' GB'
	text += '\nDownloaded size: %.2f' % (DownloadedSize) + unit

	# add average download speed
	DownloadedSizeMB = float(nzbGroup['DownloadedSizeMB'])
	DownloadTimeSec = float(nzbGroup['DownloadTimeSec'])
	if DownloadTimeSec > 0: # check x/0 errors
		avespeed = (DownloadedSizeMB/DownloadTimeSec) # MB/s
		unit = ' MB/s'
		if avespeed < 1:
			avespeed = avespeed * 1024 # KB/s
			unit = ' KB/s'
		text += '\nAverage download speed: %.2f' % (avespeed) + unit

	def format_time_sec(sec):
		Hour = sec/3600
		Min = (sec - (sec/3600)*3600)/60
		Sec = (sec - (sec/3600)*3600)%60
		return '%d:%02d:%02d' % (Hour,Min,Sec)

	# add times
	text += '\nTotal time: ' + format_time_sec(int(nzbGroup['DownloadTimeSec']) + int(nzbGroup['PostTotalTimeSec']))
	text += '\nDownload time: ' + format_time_sec(int(nzbGroup['DownloadTimeSec']))
	text += '\nVerification time: ' + format_time_sec(int(nzbGroup['ParTimeSec']) - int(nzbGroup['RepairTimeSec']))
	text += '\nRepair time: ' + format_time_sec(int(nzbGroup['RepairTimeSec']))
	text += '\nUnpack time: ' + format_time_sec(int(nzbGroup['UnpackTimeSec']))

# add list of downloaded files
files = False
if os.environ.get('NZBPO_FILELIST') == 'yes':
	text += '\n\nFiles:'
	for dirname, dirnames, filenames in os.walk(os.environ['NZBPP_DIRECTORY']):
		for filename in filenames:
			text += '\n' + os.path.join(dirname, filename)[len(os.environ['NZBPP_DIRECTORY']) + 1:]
			files = True
	if not files:
		text += '\n<no files found>'

# add _brokenlog.txt (if exists)
if os.environ.get('NZBPO_BROKENLOG') == 'yes':
	brokenlog = '%s/_brokenlog.txt' % os.environ['NZBPP_DIRECTORY']
	if os.path.exists(brokenlog):
		text += '\n\nBrokenlog:\n' + open(brokenlog, 'r').read().strip()

# add post-processing log
if os.environ.get('NZBPO_POSTPROCESSLOG') == 'Always' or \
	(os.environ.get('NZBPO_POSTPROCESSLOG') == 'OnFailure' and not success):
	# To get the post-processing log we call method "postqueue", which returns
	# the list of post-processing job.
	# The first item in the list is current job. This item has a field 'Log',
	# containing an array of log-entries.
	
	# Call remote method 'postqueue'. The only parameter tells how many log-entries to return as maximum.
	postqueue = server.postqueue(10000)
	
	# Get field 'Log' from the first post-processing job
	log = postqueue[0]['Log']
	
	# Now iterate through entries and save them to message text
	if len(log) > 0:
		text += '\n\nPost-processing log:';
		for entry in log:
			text += '\n%s\t%s\t%s' % (entry['Kind'], datetime.datetime.fromtimestamp(int(entry['Time'])), entry['Text'])

# Create message
msg = MIMEText(text)
msg['Subject'] = subject
msg['From'] = os.environ['NZBPO_FROM']
msg['To'] = os.environ['NZBPO_TO']
msg['Date'] = datetime.datetime.utcnow().strftime("%a, %d %b %Y %H:%M:%S +0000")
msg['X-Application'] = 'NZBGet'

# Send message
print('[DETAIL] Sending E-Mail')
sys.stdout.flush()
try:
	smtp = smtplib.SMTP(os.environ['NZBPO_SERVER'], os.environ['NZBPO_PORT'])

	if os.environ['NZBPO_ENCRYPTION'] == 'yes':
		smtp.starttls()
	
	if os.environ['NZBPO_USERNAME'] != '' and os.environ['NZBPO_PASSWORD'] != '':
		smtp.login(os.environ['NZBPO_USERNAME'], os.environ['NZBPO_PASSWORD'])
	
	smtp.sendmail(os.environ['NZBPO_FROM'], os.environ['NZBPO_TO'], msg.as_string())
	
	smtp.quit()
except Exception as err:
	print('[ERROR] %s' % err)
	sys.exit(POSTPROCESS_ERROR)

# All OK, returning exit status 'POSTPROCESS_SUCCESS' (int <93>) to let NZBGet know
# that our script has successfully completed.
sys.exit(POSTPROCESS_SUCCESS)
