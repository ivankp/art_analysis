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
    with open("dl/vangogh/"+local_filename, 'wb') as f:
        for chunk in r.iter_content(chunk_size=1024): 
            if chunk: # filter out keep-alive new chunks
                f.write(chunk)
                f.flush()
    return local_filename

# start a session ##############################################
sesh = requests.Session()

index = "http://www.vangoghgallery.com"

page = sesh.get(index+"/catalog/Painting/")
root = lxml.html.fromstring(page.text)

links = root.xpath('//*[@id="middle"]/table[@class="bodymainsmall"]/tr/td[2]/a')
#links = root.xpath('//*[@id="middle"]/table[2]/tbody/tr[2]/td[2]/a')

for a in links:
  page = sesh.get(index+a.attrib["href"])
  root = lxml.html.fromstring(page.text)
  img = root.xpath('//*[@id="artworkImage"]/tbody/tr/td/img')
  if len(img)==0: print "No img found in %s" % a.attrib["href"]
  else:
    url = index+img[0].attrib["src"]
    print url
    download_file(url)

print "Num links: %d" % len(links)

