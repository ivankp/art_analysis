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

index = "http://www.wikiart.org/"

num_dl = 0

for p in range(25,34):
  page = sesh.get(index+'en/vincent-van-gogh/mode/all-paintings-by-alphabet/%d' % p)
  root = lxml.html.fromstring(page.text)

  links = root.xpath('//a[@class="small rimage"]')

  for a in links:
    num_dl += 1
    page = sesh.get(index+a.attrib["href"])
    root = lxml.html.fromstring(page.text)
    img = root.xpath('//a[@id="paintingImage"]')
    if len(img)==0: print "No img found in %s" % a.attrib["href"]
    else:
      url = img[0].attrib["href"]
      print "%d : %s" % (num_dl, url)
      download_file(url)

# 1440
