#!/usr/bin/env python

import sys
import requests
import lxml.html
from lxml import etree

################################################################
def download_file(url):
    local_filename = url.split('/')[-1]
    # NOTE the stream=True parameter
    r = requests.get(url, stream=True)
    with open("dl/picasso/"+local_filename, 'wb') as f:
        for chunk in r.iter_content(chunk_size=1024): 
            if chunk: # filter out keep-alive new chunks
                f.write(chunk)
                f.flush()
    return local_filename

# start a session ##############################################
sesh = requests.Session()

index = "http://www.pablopicasso.org"

page = sesh.get(index+"/picasso-paintings.jsp")
root = lxml.html.fromstring(page.text)

links = root.xpath('//*[@id="art-main"]/div[2]/div[10]/div[3]/div/div[2]/div/div[10]/div[1]/div[2]/div/table/tbody/tr/td[1]/a')

for a in links:
  url = index+"/images/paintings"+a.attrib["href"].split('.')[0]+".jpg"
  print url
  download_file(url)

print "Num links: %d" % len(links)

