from __future__ import unicode_literals
#-*- coding:utf-8 -*-

import socket
import subprocess
import sys
import re, urllib, os, sys, urllib2
import MySQLdb
import youtube_dl

urlopen = urllib2.urlopen
encode = urllib.urlencode
retrieve = urllib.urlretrieve
cleanup = urllib.urlcleanup()

reload(sys)
sys.setdefaultencoding('utf-8')

db = MySQLdb.connect("localhost", "monitor", "password", "temps")
curs=db.cursor()

HOST = "127.0.0.2"
PORT = 19998
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
print ('Socket created')
s.bind((HOST, PORT))
print ('Socket bind complete')
s.listen(1)
print ('Socket now listening')

class MyLogger(object):
	def debug(self, msg):
		pass
	def warning(self, msg):
	 	pass
	def error(self, msg):
	 	print(msg)

def my_hook(d):
	if d['status'] == 'finished':
		print('Done downloading, now converting ...')

def video_title(url):
	try:
		webpage = vrlopen(url).read()
		title = str(webpage).split('<title>')[1].split('</title>')[0]
	except:
		title = 'Youtube Song'
	return title


def down_load(song, conn):
	if "youtube.com/" not in song:
		try:
			query_string = encode({"search_query" : song})
			html_content = urlopen("https://www.youtube.com/results?" + query_string)
			search_results = re.findall(r'href=\"\/watch\?v=(.{11})',html_content.read())
		except:
			print('Network Error')
			return None
#		command = "youtube-dl -cit --embed-thumbnail --no-warnings --extract-audio --audio-format mp3 " + search_results[0]
#		print('song : %s ' %song)
		ydl_opts = {
			'outtmpl':'/home/pi/Desktop/music_list/%(id)s.%(ext)s',
			'format': 'worstaudio/worst',
			'extractaudio': True,
			'noplaylist': True,
			'postprocessors':[{
				'key': 'FFmpegExtractAudio',
				'preferredcodec': 'wav',
				'preferredquality': '1',
			}],
			'logger': MyLogger(),
			'progress_hooks': [my_hook],
		}
		
		with youtube_dl.YoutubeDL(ydl_opts) as ydl:
			info_dict = ydl.extract_info(search_results[0], download=False)
			audio_title = info_dict.get('title', None)
			id = info_dict.get('id', None)
			print(info_dict['title'])
#		ydl.download([search_results[0]])
		print('file_name : %s'% audio_title)
#		song = video_title(song)
		dir_name = "/home/pi/Desktop/music_list/" + id + ".wav"
		print(dir_name)
		sql = "SELECT count(*) as count FROM list WHERE song = '%s'" % id
		try:
			curs.execute(sql)
			rows = curs.fetchall()
			for row in rows:
				if row[0] == 0:
					print "down"
					buf = str("0").encode('utf-8')
					conn.send(buf)
					"""
					try:
						query_string = encode({"search_query" : song})
						html_content = urlopen("https://www.youtube.com/results?" + query_string)
						search_results = re.findall(r'href=\"\/watch\?v=(.{11})',html_content.read())
					except:
						print('Network Error')
						return None

					ydl_opts = {
						'outtmpl':'/home/pi/Desktop/music_list/%(id)s.%(ext)s',
						'format': 'worstaudio/worst',
						'extractaudio': True,
						'noplaylist': True,
						'postprocessors':[{
							'key': 'FFmpegExtractAudio',
							'preferredcodec': 'wav',
							'preferredquality': '1',
						}],
						'logger': MyLogger(),
						'progress_hooks': [my_hook],
					}
					"""
					with youtube_dl.YoutubeDL(ydl_opts) as ydl:
						info_dict = ydl.extract_info(search_results[0], download=False)
						audio_title = info_dict.get('title', None)
						id = info_dict.get('id', None)
						print(info_dict['title'])
						ydl.download([search_results[0]])
					sql = "INSERT INTO list(song,filename) VALUES('%s','%s')" % (id, dir_name )
					try:
						curs.execute(sql)
						db.commit()
					except:
						db.rollback()

				else :
					print "play"
					buf = str("1").encode('utf-8')
					conn.send(buf)
		except MySQLdb.Error, e:
			print "select error"
			print e.args[1]
			db.rollback()
		
		buf = str(dir_name.encode('utf-8'))
		conn.send(buf)
	else:
#		command = "youtube-dl -cit --embed-thumbnail --no-warnings --extract-audio --audio-format mp3 " + song[song.find("=")+1:]
		song = video_title(song)
#	try:
#		print('Complete downloading %s'% song)
#		os.system(command)
#	except:
#		print('Error downloading %s'% song)
		return None

def main():
	while True:
		conn, addr = s.accept()
		print("conneted by", addr)

		data=conn.recv(500)
#		data = str(data).encode('utf-8')
		data=data.decode("utf").strip()
		print 'python : '+data
#data=data.decode("utf").strip()
		if not data: break
		print("Received: "+data)
		down_load(data, conn)
#		buf = str("c.wav").encode('utf-8')
#		conn.send(buf)
		conn.close
	s.close()


main()
