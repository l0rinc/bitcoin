#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Count Bitcoin Core contributors from git history.

The script walks commits from newest to oldest starting at the current
checkout by default. Merge counts are shown but omitted from ranking unless
--include-merges is passed. Commits that only touch imported third-party
subtrees are skipped unless --include-external is passed.
"""

from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from email.utils import getaddresses, parseaddr
import json
import re
import subprocess
import sys
import unicodedata


# Commits that only touch these imported subtrees are not Bitcoin Core
# contributions for this report.
EXTERNAL_PREFIXES = (
    "src/crc32c/",
    "src/leveldb/",
    "src/minisketch/",
    "src/secp256k1/",
    "src/univalue/",
)


# Same-user database. Add entries by appending strings to the list for a
# canonical display name. Matching is case-insensitive and accent-insensitive;
# parsed email addresses are matched directly so the same email cannot split
# across contributors.
SAME_AUTHORS = {
    "/dev/fd0": [
        "/dev/fd0 <alicexbt@protonmail.com>",
        "1440000bytes <alicexbt@protonmail.com>",
    ],
    "0xb10c": [
        "0xb10c",
        "0xB10C",
        "b10c",
        "0xb10c@gmail.com",
        "19157360+0xB10C@users.noreply.github.com",
        "b10c@b10c.me",
        "0xb10c <0xb10c@gmail.com>",
        "0xB10C <19157360+0xB10C@users.noreply.github.com>",
        "0xb10c <b10c@b10c.me>",
    ],
    "0xree": [
        "0xree",
    ],
    "10xcryptodev": [
        "10xcryptodev <10xcryptodev@gmail.com>",
    ],
    "1Il1": [
        "1Il1 <33648696+1Il1@users.noreply.github.com>",
    ],
    "21E14": [
        "21E14 <21xe14@gmail.com>",
    ],
    "22388o⚡️": [
        "22388o⚡️ <83122757+22388o@users.noreply.github.com>",
    ],
    "251": [
        "251",
        "251 <13120787+251labs@users.noreply.github.com>",
        "251 <13120787+251Labs@users.noreply.github.com>",
        "251 <13120787+l2a5b1@users.noreply.github.com>",
    ],
    "4d55397500": [
        "4d55397500 <svjk24@gmail.com>",
    ],
    "532479301": [
        "532479301 <532479301@qq.com>",
    ],
    "Aaron Clauson": [
        "Aaron Clauson",
        "Aaron Clauson <aaron.clauson@gmail.com>",
        "Aaron Clauson <aaron@sipsorcery.com>",
    ],
    "Aaron Golliver": [
        "Aaron Golliver <aaron.golliver@gmail.com>",
    ],
    "Aaron Hook": [
        "Aaron Hook <ahook@protonmail.com>",
    ],
    "aaron-hanson": [
        "aaron-hanson <archaeal@gmail.com>",
    ],
    "Abraham Jewowich": [
        "Abraham Jewowich <abuse@loljews.com>",
    ],
    "Abubakar Sadiq Ismail": [
        "Abubakar Sadiq Ismail <48946461+ismaelsadeeq@users.noreply.github.com>",
        "ismaelsadeeq",
        "ismaelsadeeq <abubakarsadiqismail@proton.me>",
        "ismaelsadeeq <ask4ismailsadiq@gmail.com>",
    ],
    "accraze": [
        "accraze <accraze@gmail.com>",
    ],
    "Adam Brown": [
        "Adam Brown <adam@deftnerd.com>",
    ],
    "Adam Jonas": [
        "Adam Jonas <jonas@chaincode.com>",
    ],
    "Adam Langley": [
        "Adam Langley <agl@google.com>",
    ],
    "Adam Soltys": [
        "Adam Soltys <asoltys@gmail.com>",
    ],
    "Adam Stein": [
        "Adam Stein <adaminsky@gmail.com>",
    ],
    "Adam Weiss": [
        "Adam Weiss <adam@signal11.com>",
    ],
    "Addy Yeow": [
        "Addy Yeow <ayeowch@gmail.com>",
        "ayeowch <ayeowch@gmail.com>",
    ],
    "Adlai Chandrasekhar": [
        "Adlai Chandrasekhar <adlai.chandrasekhar@gmail.com>",
    ],
    "adlawren": [
        "adlawren <adlawren010@gmail.com>",
    ],
    "Adrian-Stefan Mares": [
        "Adrian-Stefan Mares <a.mares@student.tue.nl>",
    ],
    "Afanti": [
        "Afanti <127061691+threewebcode@users.noreply.github.com>",
    ],
    "Ahmad Kazi": [
        "Ahmad Kazi <plaxton@users.noreply.github.com>",
    ],
    "aideca": [
        "aideca <aideca@users.noreply.github.com>",
    ],
    "Aitor Pazos": [
        "Aitor Pazos <mail@aitorpazos.es>",
    ],
    "aitorjs": [
        "aitorjs <aitiba@gmail.com>",
    ],
    "akankshakashyap": [
        "akankshakashyap <akankshakashyap378@gmail.com>",
    ],
    "Akio Nakamura": [
        "Akio Nakamura <nakamura@dgtechnologies.co.jp>",
    ],
    "Akira Takizawa": [
        "Akira Takizawa <akx20000@protonmail.com>",
    ],
    "Albert": [
        "Albert <github@albert.sh>",
    ],
    "Alejandro Avilés": [
        "Alejandro Avilés <omegak@gmail.com>",
    ],
    "Alex": [
        "Alex <alex>",
    ],
    "Alex B": [
        "Alex B <paraipanakos@gmail.com>",
    ],
    "Alex Groce": [
        "agroce <agroce@gmail.com>",
        "Alex Groce <agroce@gmail.com>",
    ],
    "Alex Morcos": [
        "Alex Morcos <morcos@chaincode.com>",
    ],
    "Alex van der Peet": [
        "Alex van der Peet <alex.van.der.peet@gmail.com>",
    ],
    "Alex Vear": [
        "Alex Vear <axvr@users.noreply.github.com>",
    ],
    "Alex Waters": [
        "Alex Waters <AmpedAl@Gmail.com>",
    ],
    "Alex Willmer": [
        "Alex Willmer <alex@moreati.org.uk>",
    ],
    "Alexander Jeng": [
        "Alexander Jeng <alexanderjeng@gmail.com>",
    ],
    "Alexander Kjeldaas": [
        "Alexander Kjeldaas <alexander.kjeldaas@gmail.com>",
    ],
    "Alexander Leishman": [
        "Alexander Leishman <leishman3@gmail.com>",
    ],
    "Alexander Regueiro": [
        "Alexander Regueiro <alexreg@me.com>",
    ],
    "Alexander Wiederin": [
        "Alexander Wiederin <alex@wiederin.xyz>",
    ],
    "Alexey Ivanov": [
        "Alexey Ivanov <alexey.ivanes@gmail.com>",
    ],
    "Alexey Poghilenkov": [
        "Alexey Poghilenkov <leshiy12345678@gmail.com>",
    ],
    "Alexey Vesnin": [
        "Alexey Vesnin <serxis@gmail.com>",
    ],
    "Alfie John": [
        "Alfie John <alfie@alfie.wtf>",
    ],
    "Alfonso Roman Zubeldia": [
        "Alfonso Roman Zubeldia",
        "Alfonso Roman Zubeldia <19962151+alfonsoromanz@users.noreply.github.com>",
        "Alfonso Roman Zubeldia <alfonsoromanz24@gmail.com>",
    ],
    "Ali Sherief": [
        "Ali Sherief <ali@notatether.com>",
    ],
    "Alice Wonder": [
        "Alice Wonder <github@domblogger.net>",
    ],
    "Alin Rus": [
        "Alin Rus <alin@fsck.ro>",
    ],
    "Alistair Buxton": [
        "Alistair Buxton <a.j.buxton@gmail.com>",
    ],
    "Alistair Mann": [
        "Alistair Mann <al+tfl@pectw.net>",
    ],
    "Allan Doensen": [
        "Allan Doensen <allan@doensen.com>",
    ],
    "Alon Muroch": [
        "Alon Muroch <alonmuroch@gmail.com>",
    ],
    "Altoidnerd": [
        "Altoidnerd",
        "altoidnerd",
        "allenmajs1@gmail.com",
        "altoidnerd.btc@gmail.com",
        "altoidnerd <allenmajs1@gmail.com>",
        "Altoidnerd <allenmajs1@gmail.com>",
        "Altoidnerd <altoidnerd.btc@gmail.com>",
    ],
    "Alyssa": [
        "Alyssa <orbitalturtle@protonmail.com>",
    ],
    "am-sq": [
        "am-sq <amangla@squareup.com>",
    ],
    "Amadeusz Pawlik": [
        "Amadeusz Pawlik <amadeusz.pawlik@getinge.com>",
        "amadeuszpawlik <apawlik@protonmail.com>",
        "apawlik <amadeusz.pawlik@getinge.com>",
    ],
    "Amir Abrams": [
        "Amir Abrams",
        "amirabrams",
        "aabrams@myharmoniq.com",
        "amirabrams@mail.com",
        "AmirAbrams@users.noreply.github.com",
        "Amir Abrams <aabrams@myharmoniq.com>",
        "AmirAbrams <aabrams@myharmoniq.com>",
        "Amir Abrams <AmirAbrams@users.noreply.github.com>",
        "amirabrams <amirabrams@mail.com>",
    ],
    "Amir Ghorbanian": [
        "Amir Ghorbanian <deanghorbanian@gatech.edu>",
    ],
    "Amir Yalon": [
        "Amir Yalon <git@please.nospammail.net>",
    ],
    "amisha": [
        "amisha <amishhhaaaa@gmail.com>",
    ],
    "Amiti Uttarwar": [
        "Amiti Uttarwar",
        "Amiti Uttarwar <amiti.uttarwar@coinbase.com>",
        "Amiti Uttarwar <amiti@uttarwar.org>",
    ],
    "ANAVHEOBA": [
        "ANAVHEOBA <wisdomabraham92@gmail.com>",
    ],
    "Anders Øyvind Urke-Sætre": [
        "Anders Øyvind Urke-Sætre <andersoyvind@gmail.com>",
    ],
    "Anditto Heristyo": [
        "Anditto Heristyo <anditto.heristyo@gmail.com>",
    ],
    "Andras Elso": [
        "Andras Elso <elso.andras@gmail.com>",
    ],
    "Andre Alves": [
        "Andre <andremralves@gmail.com>",
        "Andre Alves <andremralves@gmail.com>",
    ],
    "Andrea Comand": [
        "Andrea Comand <andrea@comand.me>",
    ],
    "Andrea D'Amore": [
        "Andrea D'Amore <anddam@brapi.net>",
    ],
    "Andreas Kouloumos": [
        "Andreas Kouloumos <kouloumosa@gmail.com>",
        "kouloumos <kouloumosa@gmail.com>",
    ],
    "Andreas Schildbach": [
        "Andreas Schildbach <andreas@schildbach.de>",
    ],
    "Andres G. Aragoneses": [
        "Andres G. Aragoneses",
        "Andres G. Aragoneses <knocte@gmail.com>",
        "Andrés G. Aragoneses <knocte@gmail.com>",
    ],
    "Andrew Poelstra": [
        "Andrew Poelstra",
        "Andrew Poelstra <apoelstra@wpsoftware.net>",
        "Andrew Poelstra <asp11@sfu.ca>",
    ],
    "Andrew Toth": [
        "Andrew Toth <andrewstoth@gmail.com>",
        "andrewtoth <andrewstoth@gmail.com>",
    ],
    "Andrey Alekseenko": [
        "Andrey <al42and@gmail.com>",
        "Andrey Alekseenko <al42and@gmail.com>",
    ],
    "Andriy Voskoboinyk": [
        "Andriy Voskoboinyk <andriivos@gmail.com>",
    ],
    "Andy Alness": [
        "Andy Alness <andy@coinbase.com>",
    ],
    "Ang Iong Chun": [
        "Ang Iong Chun <angiongchun@gmail.com>",
    ],
    "AngusP": [
        "AngusP <angus@toaster.cc>",
    ],
    "Anonymous": [
        "Anonymous <none@anon>",
    ],
    "anouar kappitou": [
        "anouar kappitou <anoirkapp@gmail.com>",
    ],
    "Anthony Fieroni": [
        "Anthony Fieroni <bvbfan@abv.bg>",
    ],
    "Anthony Ronning": [
        "Anthony Ronning <anthonyronning@gmail.com>",
    ],
    "Anthony Towns": [
        "Anthony Towns <aj@erisian.com.au>",
    ],
    "Antoine Le Calvez": [
        "Antoine Le Calvez <antoine@alc.io>",
    ],
    "Antoine Poinsot": [
        "Antoine Poinsot",
        "darosior",
        "darosior@protonmail.com",
        "darosior@gmail.com",
        "mail@antoinep.com",
        "Antoine Poinsot <darosior@protonmail.com>",
        "Antonie Poinsot <darosior@protonmail.com>",
        "darosior <darosior@protonmail.com>",
        "Antoine Poinsot <mail@antoinep.com>",
        "darosior <darosior@gmail.com>",
    ],
    "Antoine Riard": [
        "Antoine Riard",
        "ariard",
        "antoine.riard@gmail.com",
        "ariard@student.42.fr",
        "dev@ariard.me",
        "Antoine Riard <antoine.riard@gmail.com>",
        "ariard <antoine.riard@gmail.com>",
        "Antoine Riard <ariard@student.42.fr>",
        "Antoine Riard <dev@ariard.me>",
    ],
    "Anton A": [
        "Anton A <contact@antonphp.com>",
    ],
    "antonio-fr": [
        "antonio-fr <anferron@gmail.com>",
    ],
    "Antti Majakivi": [
        "anduck <anduck@users.noreply.github.com>",
        "Antti Majakivi <anduck@users.noreply.github.com>",
    ],
    "ANtutov": [
        "ANtutov <tutovanton26@gmail.com>",
    ],
    "Anurag chavan": [
        "Anurag chavan <118217089+anuragchvn-blip@users.noreply.github.com>",
    ],
    "APerson241": [
        "APerson241 <setup.pyc@gmail.com>",
    ],
    "apitko": [
        "apitko <81133459+apitko@users.noreply.github.com>",
    ],
    "araspitzu": [
        "araspitzu <a.raspitzu@gmail.com>",
    ],
    "Ari": [
        "Ari <52634803+theblackmace@users.noreply.github.com>",
    ],
    "ariel": [
        "ariel <ariel@ficticio.com>",
    ],
    "Arnab Sen": [
        "Arnab Sen <arnabsen1729@gmail.com>",
    ],
    "Arnav Singh": [
        "Arnav Singh <arnavion@gmail.com>",
    ],
    "Arne Brutschy": [
        "Arne Brutschy <abrutschy@xylon.de>",
    ],
    "arowser": [
        "arowser <arowser@gmail.com>",
        "daniel <arowser@gmail.com>",
    ],
    "Arvid Norberg": [
        "Arvid Norberg <arvid@blockstream.io>",
    ],
    "ArvinFarrelP": [
        "ArvinFarrelP <99516005+ArvinFarrelP@users.noreply.github.com>",
    ],
    "Aseem Sood": [
        "Aseem Sood <asood123@yahoo.com>",
    ],
    "Ash Manning": [
        "Ash Manning <10554686+A-Manning@users.noreply.github.com>",
    ],
    "Ashley Holman": [
        "Ashley Holman <dscvlt@gmail.com>",
    ],
    "AtsukiTak": [
        "AtsukiTak <takatomgoo@gmail.com>",
    ],
    "Aurèle Oulès": [
        "Aurèle Oulès",
        "Aurèle Oulès <aurele@oules.com>",
        "Aurèle Oulès <hello@aureleoules.com>",
    ],
    "Ava Barron": [
        "Ava Barron <mztriz@gmail.com>",
    ],
    "Ava Chow": [
        "Ava Chow",
        "Andrew Chow",
        "achow101",
        "achow101@gmail.com",
        "achow101-github@achow101.com",
        "github@achow101.com",
        "Andrew Chow <achow101-github@achow101.com>",
        "Andrew Chow <achow101@gmail.com>",
        "Andrew Chow <github@achow101.com>",
        "Ava Chow <github@achow101.com>",
        "Andrew <achow101@gmail.com>",
        "Andrew C <achow101@gmail.com>",
        "merge-script <github@achow101.com>",
    ],
    "avirgovi": [
        "avirgovi <avirgovi@cisco.com>",
    ],
    "Awemany": [
        "Awemany <awemany@protonmail.com>",
    ],
    "Ayush Sharma": [
        "Ayush Sharma <mrayushs933@gmail.com>",
    ],
    "Ayush Singh": [
        "Ayush Singh",
        "Ayush Singh <ayush.singh1@meesho.com>",
        "Ayush Singh <ayushsingh.as1700@gmail.com>",
    ],
    "azeteki": [
        "azeteki <azeteki@safe-mail.net>",
    ],
    "azuchi": [
        "azuchi <azuchi@haw.co.jp>",
    ],
    "b-l-u-e": [
        "b-l-u-e <winnie.gitau282@gmail.com>",
    ],
    "b0xxer": [
        "b0xxer <barry@a9.local>",
    ],
    "b6393ce9-d324-4fe1-996b-acf82dbc3d53": [
        "b6393ce9-d324-4fe1-996b-acf82dbc3d53 <m8r-emkdvd@mailinator.com>",
    ],
    "Baas": [
        "Baas <matthew.baas@gmail.com>",
    ],
    "bajjer": [
        "bajjer <bajjer@bajjer.xyz>",
    ],
    "Bardi Harborow": [
        "Bardi Harborow <bardi_harborow@yahoo.com.au>",
    ],
    "Barry Deeney": [
        "Barry Deeney <mxaddict@codedmaster.com>",
    ],
    "bboot": [
        "bboot <bboot@cisco.com>",
    ],
    "Ben Carman": [
        "Ben Carman <benthecarman@live.com>",
        "benthecarman <benthecarman@live.com>",
    ],
    "Ben Holden-Crowther": [
        "Ben Holden-Crowther",
        "Ben Holden-Crowther <benhc123@users.noreply.github.com>",
        "Ben Holden-Crowther <benhc@live.co.uk>",
    ],
    "Ben Schroth": [
        "Ben Schroth <ben@styng.social>",
    ],
    "Ben Westgate": [
        "Ben Westgate",
        "Ben Westgate <73506583+BenWestgate@users.noreply.github.com>",
        "Ben Westgate <BenWestgate@protonmail.com>",
    ],
    "Ben Woosley": [
        "Empact",
        "Ben Woosley <ben.woosley@gmail.com>",
    ],
    "Benedict Chan": [
        "Benedict Chan <bencxr@fragnetics.com>",
    ],
    "benk10": [
        "benk10 <ben.kaufman10@gmail.com>",
    ],
    "Benoit Verret": [
        "Benoit Verret <verret.benoit@gmail.com>",
    ],
    "bensig": [
        "bensig <bensig@gmail.com>",
    ],
    "Bernhard M. Wiedemann": [
        "Bernhard M. Wiedemann <bwiedemann@suse.de>",
    ],
    "Bezdrighin": [
        "Bezdrighin <mbbezdri@3c22fbe8ae1b.ant.amazon.com>",
    ],
    "bikinibabe": [
        "bikinibabe <amberwelch@unomaha.edu>",
    ],
    "Billy Garrison": [
        "Billy Garrison <billygarrison.btc@gmail.com>",
    ],
    "billymcbip": [
        "billymcbip <245003547+billymcbip@users.noreply.github.com>",
    ],
    "Bitcoin Hodler": [
        "Bitcoin Hodler <bitcoinhodler@safe-mail.net>",
        "bitcoinhodler <31543633+bitcoinhodler@users.noreply.github.com>",
    ],
    "bitcoin-core-merge-script": [
        "merge-script <90386131+bitcoin-core-merge-script@users.noreply.github.com>",
    ],
    "BitcoinTsunami": [
        "BitcoinTsunami",
    ],
    "Blake Jakopovic": [
        "Blake Jakopovic <blake.jakopovic@gmail.com>",
    ],
    "Blitzboom": [
        "Blitzboom <anon@none>",
        "Danube <anon@none>",
        "m0ray <anon@none>",
        "mewantsbitcoins <anon@none>",
    ],
    "Block Mechanic": [
        "Block Mechanic <blockmecha@gmail.com>",
        "BlockMechanic <blockmecha@gmail.com>",
    ],
    "Bob McElrath": [
        "Bob McElrath",
        "Bob McElrath <bob@mcelrath.org>",
        "Bob McElrath <bob_git@mcelrath.org>",
    ],
    "Boris Nagaev": [
        "Boris Nagaev <bnagaev@gmail.com>",
    ],
    "bpay": [
        "bpay <bpay@users.noreply.github.com>",
    ],
    "Brandon Dahler": [
        "Brandon Dahler <brandon.dahler@gmail.com>",
    ],
    "Brandon Odiwuor": [
        "Brandon Odiwuor <brandon.odiwuor@gmail.com>",
    ],
    "Brandon Ruggles": [
        "Brandon Ruggles <brandonrninefive@gmail.com>",
    ],
    "Braydon Fuller": [
        "Braydon Fuller <braydon@bitpay.com>",
    ],
    "Brian Deery": [
        "Brian Deery <brian@factom.org>",
    ],
    "Brian Liotti": [
        "Brian Liotti <bliotti@protonmail.com>",
    ],
    "Brian McMichael": [
        "Brian McMichael <brian@brianmcmichael.com>",
    ],
    "Brian Solon": [
        "Brian Solon <solon@users.noreply.github.com>",
    ],
    "brianddk": [
        "brianddk <brianddk@users.noreply.github.com>",
    ],
    "Brotcrunsher": [
        "Brotcrunsher <Brotcrunsher@hotmail.de>",
    ],
    "Bruno Garcia": [
        "Bruno Garcia",
        "brunoerg",
        "brunoely.gc@gmail.com",
        "bgarcia@3xbit.com.br",
        "bruno <bgarcia@3xbit.com.br>",
        "bruno <brunoely.gc@gmail.com>",
        "Bruno Garcia <brunoely.gc@gmail.com>",
        "brunoerg <brunoely.gc@gmail.com>",
    ],
    "Bryan Bishop": [
        "Bryan Bishop <kanzure@gmail.com>",
    ],
    "brydinh": [
        "brydinh <bdinh98@gmail.com>",
    ],
    "bstin": [
        "bstin <barry.github@capsmx.com>",
    ],
    "BtcDrak": [
        "BtcDrak",
        "฿tcDrak",
        "btcdrak@gmail.com",
        "btcdrak@users.noreply.github.com",
        "drak@zikula.org",
        "BtcDrak <btcdrak@gmail.com>",
        "Drak <drak@zikula.org>",
        "฿tcDrak <btcdrak@users.noreply.github.com>",
    ],
    "Buck Perley": [
        "Buck Perley <bucko.perley@gmail.com>",
    ],
    "buddilla": [
        "buddilla <buddilla@users.noreply.github.com>",
    ],
    "Bue-von-hon": [
        "Bue-von-hon <dkssudvn2@gmail.com>",
    ],
    "Bufo": [
        "Bufo <bufo24@users.noreply.github.com>",
    ],
    "burger2": [
        "burger2 <birger.hedman@gmail.com>",
    ],
    "CaesarCoder": [
        "CaesarCoder <caesrcd@tutamail.com>",
    ],
    "Caleb Delisle": [
        "Caleb Delisle",
        "cjdelisle <calebdelisle@lavabit.com>",
    ],
    "calebogden": [
        "calebogden <email@calebogden.com>",
    ],
    "Calin Culianu": [
        "Calin Culianu <calin.culianu@gmail.com>",
    ],
    "CallMeMisterOwl": [
        "CallMeMisterOwl <denizhasler@outlook.de>",
    ],
    "Calvin Kim": [
        "Calvin Kim <calvin@kcalvinalvin.info>",
    ],
    "Calvin Owens": [
        "Calvin Owens <jcalvinowens@gmail.com>",
    ],
    "Calvin Tam": [
        "Calvin Tam <calvinyhtam@gmail.com>",
    ],
    "CAnon": [
        "CAnon <CAnon@example.com>",
    ],
    "cardpuncher": [
        "cardpuncher <mauron@vmail.me>",
    ],
    "Carl Dong": [
        "Carl Dong",
        "Carl Dong <accounts@carldong.me>",
        "Carl Dong <contact@carldong.me>",
        "Carl Dong <git@carldong.me>",
    ],
    "Carlo Alberto Ferraris": [
        "Carlo Alberto Ferraris <github@cafxx.strayorange.com>",
    ],
    "Carlo Antinarella": [
        "Carlo Antinarella <carloantinarella@users.noreply.github.com>",
    ],
    "Carlos Pizarro": [
        "Carlos Pizarro <kr105@kr105.com>",
    ],
    "Carnhof Daki": [
        "Carnhof Daki <carnhofdaki@gmail.com>",
        "Daki Carnhof <carnhofdaki@gmail.com>",
    ],
    "Casey Carter": [
        "Casey Carter <cacarter@microsoft.com>",
    ],
    "Casey Rodarmor": [
        "Casey Rodarmor <casey@rodarmor.com>",
    ],
    "Cédric Félizard": [
        "Cédric Félizard <cedric@felizard.fr>",
    ],
    "cedwies": [
        "cedwies <141683552+cedwies@users.noreply.github.com>",
    ],
    "Celil": [
        "Celil <celil.kj@gmail.com>",
        "celil-kj <celil.kj@gmail.com>",
    ],
    "centaur1": [
        "centaur1 <centaur1@users.noreply.github.com>",
    ],
    "Chakib Benziane": [
        "Chakib Benziane <chakib.benz@gmail.com>",
    ],
    "Chandra Pratap": [
        "Chandra Pratap <chandrapratap3519@gmail.com>",
    ],
    "charlescharles": [
        "charlescharles <platypode@gmail.com>",
    ],
    "Charlie": [
        "Charlie <2747302+CharlieC3@users.noreply.github.com>",
    ],
    "Charlie Lee": [
        "Charlie Lee <coblee@litecoin.org>",
        "coblee <chocobo@alum.mit.edu>",
    ],
    "chinggg": [
        "chinggg <24590067+chinggg@users.noreply.github.com>",
    ],
    "Chirag Davé": [
        "Chirag Davé <c@chirag.io>",
    ],
    "Chris (none)": [
        "Chris <chris@vikki-old.(none)>",
    ],
    "Chris Abrams": [
        "Chris Abrams <mail@chrisabrams.com>",
    ],
    "Chris Arnesen": [
        "Chris Arnesen <chris.arnesen@gmail.com>",
    ],
    "Chris Beams": [
        "Chris Beams <chris@beams.io>",
    ],
    "Chris Capobianco": [
        "Chris Capobianco <chris.capobianco@greenbricklabs.com>",
    ],
    "Chris Gavin": [
        "Chris Gavin <chris@chrisgavin.me>",
    ],
    "Chris Geihsler": [
        "Chris Geihsler <chris@geihsler.net>",
    ],
    "Chris Heyes": [
        "Chris Heyes <22148308+hazeycode@users.noreply.github.com>",
    ],
    "Chris Howie": [
        "Chris Howie <me@chrishowie.com>",
    ],
    "Chris Kleeschulte": [
        "Chris Kleeschulte <chrisk@bitpay.com>",
    ],
    "Chris L": [
        "Chris L <stylesuxx@gmail.com>",
    ],
    "Chris Moore": [
        "Chris Moore <dooglus@gmail.com>",
    ],
    "Chris Stewart": [
        "Chris Stewart <stewart.chris1234@gmail.com>",
    ],
    "Chris Wheeler": [
        "Chris Wheeler <chris@haydenwheeler.com>",
    ],
    "chris-belcher": [
        "chris-belcher <chris-belcher@users.noreply.github.com>",
    ],
    "Christian Barcenas": [
        "Christian Barcenas <christian@cbarcenas.com>",
    ],
    "Christian Decker": [
        "Christian Decker",
        "Christian Decker <cdecker@tik.ee.ethz.ch>",
        "Christian Decker <decker.christian@gmail.com>",
    ],
    "Christian Gentry": [
        "Christian Gentry <christiangentry@gmail.com>",
    ],
    "Christian von Roques": [
        "Christian von Roques <roques@mti.ag>",
    ],
    "Christopher Bergqvist": [
        "Christopher Bergqvist <chris@digitalpoetry.se>",
    ],
    "Christopher Coverdale": [
        "ccdle12 <chris.coverdale24@gmail.com>",
        "Christopher Coverdale <chris.coverdale24@gmail.com>",
    ],
    "Christopher Latham": [
        "Christopher Latham <sudosurootdev@gmail.com>",
    ],
    "Christopher Sweeney": [
        "Christopher Sweeney <sweeney.chris@gmail.com>",
    ],
    "Chuck": [
        "Chuck <chuck@borboggle.com>",
    ],
    "Chuck LeDuc Díaz": [
        "Chuck LeDuc Díaz <chuck.leduc@sage.com>",
    ],
    "Chun Kuan Lee": [
        "Chun Kuan Lee <ken2812221@gmail.com>",
        "Ken Lee <ken2812221@gmail.com>",
        "ken2812221 <ken2812221@gmail.com>",
    ],
    "chungeun-choi": [
        "chungeun-choi <cucuridas@gmail.com>",
    ],
    "ci": [
        "ci <ci@ci.ci>",
    ],
    "Ciemon": [
        "Ciemon <ciemon@gmail.com>",
    ],
    "Clark Gaebel": [
        "Clark Gaebel <cgaebel@csclub.uwaterloo.ca>",
    ],
    "clashic": [
        "clashic <clashic@fullnodenocix2.clashic.cash>",
    ],
    "clashicly": [
        "clashicly <35277077+clashicly@users.noreply.github.com>",
    ],
    "Clem Taylor": [
        "Clem Taylor <clem@ossifrage.com>",
    ],
    "Clinton Christian": [
        "Clinton Christian <Clinton.Christian@me.com>",
    ],
    "Coder": [
        "Coder <161350311+MamunC0der@users.noreply.github.com>",
    ],
    "coderrr": [
        "coderrr <coderrr.contact@gmail.com>",
    ],
    "codeShark149": [
        "codeShark149 <rajarshi149@gmail.com>",
        "rajarshimaitra <rajarshi149@gmail.com>",
    ],
    "CohibAA": [
        "CohibAA <CohibAA@users.noreply.github.com>",
    ],
    "coinforensics": [
        "coinforensics <59567284+coinforensics@users.noreply.github.com>",
    ],
    "Colin Dean": [
        "Colin Dean <git@cad.cx>",
    ],
    "Conor Scott": [
        "Conor Scott <conor.r.scott.88@gmail.com>",
    ],
    "Conrado Gouvea": [
        "Conrado Gouvea <conradoplg@gmail.com>",
    ],
    "constantined": [
        "constantined <nobody@constantined.com>",
    ],
    "Corinne Dashjr": [
        "Corinne Dashjr <corinne+git@dashjr.org>",
    ],
    "Cornelius Schumacher": [
        "Cornelius Schumacher <schumacher@kde.org>",
    ],
    "Cory Fields": [
        "Cory Fields",
        "theuni",
        "coryfields.com",
        "theuni@users.noreply.github.com",
        "theuni-nospam@xbmc.org",
        "Cory Fields <cory-nospam-@coryfields.com>",
        "Cory Fields <theuni-nospam-@xbmc.org>",
        "Cory Fields <theuni@users.noreply.github.com>",
        "theuni <theuni-nospam@xbmc.org>",
    ],
    "costcould": [
        "costcould <fliter@myyahoo.com>",
    ],
    "Cozz Lovan": [
        "Cozz Lovan <cozzlovan@yahoo.com>",
    ],
    "Craig Andrews": [
        "Craig Andrews <candrews@integralblue.com>",
    ],
    "Craig Younkins": [
        "Craig Younkins <cyounkins@gmail.com>",
    ],
    "crazeteam": [
        "crazeteam <lilujing@outlook.com>",
    ],
    "Cristian Mircea Messel": [
        "Cristian Mircea Messel <mess110@gmail.com>",
    ],
    "crowning-": [
        "crowning- <crowning-@users.noreply.github.com>",
    ],
    "CryptAxe": [
        "CryptAxe <cryptaxe@gmail.com>",
    ],
    "CryptoVote": [
        "CryptoVote <cryptovote@yandex.com>",
    ],
    "ctp-tsteenholdt": [
        "ctp-tsteenholdt <tsteenholdt@cascadetechnologypartners.com>",
    ],
    "Cuong V. Nguyen": [
        "Cuong V. Nguyen <nguyencuongcl1215@gmail.com>",
    ],
    "da1sychain": [
        "da1sychain <jstdnn+grocer@protonmail.com>",
    ],
    "dabaopku": [
        "dabaopku <guocong89@gmail.com>",
    ],
    "Dag Robole": [
        "Dag Robole <dag.robole@gmail.com>",
    ],
    "Dagur Valberg Johannsson": [
        "Dagur Valberg Johannsson <dagurval@pvv.ntnu.no>",
    ],
    "Daira Hopwood": [
        "Daira Hopwood <daira@jacaranda.org>",
    ],
    "Damian Mee": [
        "bugs@meedamian.com",
        "meeDamian",
        "Damian Mee <spam@meedamian.com>",
    ],
    "Damian Williamson": [
        "Damian Williamson <willtech@live.com.au>",
    ],
    "Dan Benjamin": [
        "Dan Benjamin <danben@gmail.com>",
    ],
    "Dan Bolser": [
        "Dan Bolser <dan.bolser@gmail.com>",
    ],
    "Dan Gershony": [
        "Dan Gershony <dan.gershony@gmail.com>",
    ],
    "Dan Helfman": [
        "Dan Helfman <witten@torsion.org>",
    ],
    "Dan Loewenherz": [
        "Dan Loewenherz <dloewenherz@gmail.com>",
    ],
    "Dan Raviv": [
        "Dan Raviv <dan@soundradix.com>",
    ],
    "daniel": [
        "daniel <daniel.socials@gmail.com>",
    ],
    "Daniel Aleksandersen": [
        "Daniel Aleksandersen <code@daniel.priv.no>",
    ],
    "Daniel Cousens": [
        "Daniel Cousens",
        "Daniel Cousens <dcousens@users.noreply.github.com>",
        "Daniel Cousens <github@dcousens.com>",
    ],
    "Daniel Edgecumbe": [
        "Daniel Edgecumbe <esotericnonsense@danedgecumbe.com>",
    ],
    "Daniel Folkinshteyn": [
        "Daniel Folkinshteyn <nanotube@users.sourceforge.net>",
    ],
    "Daniel Holbert": [
        "Daniel Holbert <dholbert@cs.stanford.edu>",
    ],
    "Daniel Ingram": [
        "Daniel Ingram <ingramds@appstate.edu>",
    ],
    "Daniel Kraft": [
        "domob1812",
        "Daniel Kraft <d@domob.eu>",
    ],
    "Daniel Larimer": [
        "Daniel Larimer <dlarimer@gmail.com>",
    ],
    "Daniel McNally": [
        "Daniel McNally <mcnallydp@gmail.com>",
    ],
    "Daniel Newton": [
        "Daniel Newton <djpnewton@gmail.com>",
        "djpnewton <djpnewton@gmail.com>",
    ],
    "Daniel Pfeifer": [
        "Daniel Pfeifer <daniel@pfeifer-mail.de>",
    ],
    "Daniela Brozzoni": [
        "Daniela Brozzoni <danielabrozzoni@protonmail.com>",
    ],
    "Danny-Scott": [
        "Danny-Scott <daniel@coincorner.com>",
    ],
    "danra": [
        "danra <danra@users.noreply.github.com>",
    ],
    "Darius Parvin": [
        "Darius Parvin <darius@berkeley.edu>",
    ],
    "Darko Janković": [
        "Darko Janković <3431769+trulex@users.noreply.github.com>",
    ],
    "darksh1ne": [
        "darksh1ne <microspam@list.ru>",
    ],
    "Dave Collins": [
        "Dave Collins <davec@conformal.com>",
    ],
    "Dave Scotese": [
        "Dave Scotese <dscotese@litmocracy.com>",
        "dscotese <dscotese@litmocracy.com>",
    ],
    "DaveFromBinary": [
        "DaveFromBinary <36311895+DaveFromBinary@users.noreply.github.com>",
    ],
    "David A. Harding": [
        "David A. Harding <dave@dtrt.org>",
    ],
    "David Álvarez Rosa": [
        "David Álvarez Rosa <dalvrosa@amazon.com>",
    ],
    "David Bakin": [
        "David Bakin <david@bakins-bits.com>",
    ],
    "David FRANCOIS": [
        "David FRANCOIS <david.francois@webflows.fr>",
    ],
    "David Griffith": [
        "David Griffith <dave@661.org>",
    ],
    "David Grogan": [
        "David Grogan <dgrogan@chromium.org>",
    ],
    "David Gumberg": [
        "David Gumberg <davidzgumberg@gmail.com>",
    ],
    "David Hill": [
        "David Hill",
        "David Hill <dhill@conformal.com>",
        "David Hill <dhill@mindcry.org>",
    ],
    "David Joel Schwartz": [
        "David Joel Schwartz <davidjoelschwartz@gmail.com>",
        "JoelKatz <DavidJoelSchwartz@GMail.com>",
    ],
    "David O'Callaghan": [
        "David O'Callaghan <dave@docallag.com>",
    ],
    "David Perry": [
        "David Perry <enmaku@gmail.com>",
    ],
    "David Reikher": [
        "David Reikher <david.reikher@gmail.com>",
    ],
    "David Serrano": [
        "David Serrano <dserrano5@dserrano5.es>",
    ],
    "Dawid Spiechowicz": [
        "Dawid Spiechowicz <spiechu@gmail.com>",
    ],
    "deadmanoz": [
        "deadmanoz",
        "deadmanoz <gh@deadmanoz.xyz>",
        "deadmanoz <hones02_tunica@icloud.com>",
    ],
    "Dean Lee": [
        "Dean Lee <xslidian@gmail.com>",
    ],
    "default (none)": [
        "default <default@default-dell.(none)>",
    ],
    "Denis Lukianov": [
        "Denis Lukianov <denis@voxelsoft.com>",
    ],
    "dennsikl": [
        "dennsikl <comf000747@gmail.com>",
    ],
    "Derek Miller": [
        "Derek Miller <Derek701@users.noreply.github.com>",
        "Derek701 <brainish@gmail.com>",
    ],
    "DesWurstes": [
        "DesWurstes <DesWurstes@users.noreply.github.com>",
    ],
    "Dev Random": [
        "Dev Random <c1.github@niftybox.net>",
        "devrandom <c1.github@niftybox.net>",
    ],
    "dexX7": [
        "dexX7",
        "dexX7 <dexx@bitwatch.co>",
        "dexX7 <ugithub@bitwatch.co>",
    ],
    "Dhruv Mehta": [
        "dhruv <856960+dhruv@users.noreply.github.com>",
        "Dhruv Mehta <856960+dhruv@users.noreply.github.com>",
    ],
    "Diego Viola": [
        "Diego Viola <diego.viola@gmail.com>",
    ],
    "dimitaracev": [
        "dimitaracev <dimitaracev@protonmail.com>",
    ],
    "Dimitri": [
        "Dimitri <kvaciral@protonmail.com>",
        "Kvaciral <kvaciral@protonmail.com>",
    ],
    "Dimitris Apostolou": [
        "Dimitris Apostolou",
        "Dimitris Apostolou <dimitris.apostolou@icloud.com>",
        "rex4539 <dimitris.apostolou@icloud.com>",
        "Dimitris Apostolou <rex@MacBook-Pro-2011.local>",
    ],
    "Dimitris Tsapakidis": [
        "Dimitris Tsapakidis",
        "Dimitris Tsapakidis <dimitris@tsapakidis.com>",
        "Dimitris Tsapakidis <dtsapakidis@dtsapakidis-in-mbp.fritz.box>",
    ],
    "ditto-b": [
        "ditto-b",
        "ditto-b <ditto-b@users.noreply.github.com>",
        "ditto-b <nipun.d93+evil@gmail.com>",
    ],
    "djschnei21": [
        "djschnei21 <32646250+djschnei21@users.noreply.github.com>",
    ],
    "dllud": [
        "dllud <david.ludovino@gmail.com>",
    ],
    "Dmitry Goncharov": [
        "Dmitry Goncharov <dgoncharov@users.sf.net>",
    ],
    "Dmitry Petukhov": [
        "Dmitry Petukhov <dp@simplexum.com>",
    ],
    "Dmitry Smirnov": [
        "Dmitry Smirnov <onlyjob@member.fsf.org>",
    ],
    "Dominik Spicher": [
        "Dominik Spicher <dominik.spicher@inacta.ch>",
    ],
    "Dominyk Tiller": [
        "Dominyk Tiller <DomT4@users.noreply.github.com>",
    ],
    "Don Patterson": [
        "Don Patterson <d_j_p_3@djp3.net>",
    ],
    "Donal OConnor": [
        "Donal OConnor <donaloconnor@gmail.com>",
        "donaloconnor <donaloconnor@gmail.com>",
    ],
    "dongsamb": [
        "dongsamb <dongsamb@gmail.com>",
    ],
    "dongwook-chan": [
        "dongwook-chan <dongwook.chan@gmail.com>",
    ],
    "dontbyte": [
        "dontbyte <25500244+dontbyte@users.noreply.github.com>",
    ],
    "Doug Huff": [
        "Doug Huff <mith@jrbobdobbs.org>",
        "Douglas Huff <mith@jrbobdobbs.org>",
    ],
    "Douglas Chimento": [
        "dougEfish <dchimento@gmail.com>",
        "Douglas Chimento <dchimento@gmail.com>",
    ],
    "Douglas Roark": [
        "Douglas Roark",
        "Doug <doug@bitcoinarmory.com>",
        "Doug <joroark@vt.edu>",
        "Douglas Roark <joroark@vt.edu>",
        "Douglas Roark <doug@bloq.com>",
    ],
    "dplusplus1024": [
        "dplusplus1024 <82842780+dplusplus1024@users.noreply.github.com>",
    ],
    "DrahtBot": [
        "DrahtBot <39886733+DrahtBot@users.noreply.github.com>",
    ],
    "Drew Rasmussen": [
        "Drew Rasmussen <drew@otcxn.com>",
    ],
    "duanemoody": [
        "duanemoody <duane_moody@yahoo.com>",
    ],
    "Duncan Dean": [
        "Duncan Dean <duncangleeddean@gmail.com>",
    ],
    "Dusty Williams": [
        "Dusty Williams <dusty.wil@gmail.com>",
    ],
    "Dwayne C. Litzenberger": [
        "Dwayne C. Litzenberger <dlitz@dlitz.net>",
    ],
    "Dylan Noblesmith": [
        "Dylan Noblesmith <nobled@dreamwidth.org>",
    ],
    "Dzhelil Rufat": [
        "Dzhelil Rufat <dzhelil@gmail.com>",
    ],
    "Earlz": [
        "Earlz <earlz@earlz.net>",
    ],
    "Edil Medeiros": [
        "Edil Medeiros <jose.edil@gmail.com>",
    ],
    "Eelis": [
        "Eelis <eelis@bitonic.nl>",
    ],
    "ekzyis": [
        "ekzyis <ramdip.singhgill@gmail.com>",
    ],
    "Elias Rohrer": [
        "Elias Rohrer <rohrer@informatik.hu-berlin.de>",
    ],
    "Elichai Turkel": [
        "elichai <elichai.turkel@gmail.com>",
        "Elichai Turkel <elichai.turkel@gmail.com>",
    ],
    "Elise Schedler": [
        "Elise Schedler <eliseschedler@gmail.com>",
    ],
    "elkingtowa": [
        "elkingtowa <elkingtowa@gmail.com>",
    ],
    "Elle Mouton": [
        "Elle Mouton <elle.mouton@gmail.com>",
    ],
    "Elliot Olds": [
        "Elliot Olds <elliotolds@gmail.com>",
    ],
    "Elliott Jin": [
        "Elliott Jin",
        "Danny Lee <robot-visions@protonmail.com>",
        "Elliott Jin <robot-visions@protonmail.com>",
        "Elliott Jin <elliott.jin@gmail.com>",
    ],
    "Emanuele Cisbani": [
        "Emanuele Cisbani <emanuele.cisbani@gmail.com>",
    ],
    "Emil": [
        "Emil <emu@emuadmin.com>",
    ],
    "Emil Engler": [
        "Emil Engler",
        "Emil Engler <emilstud2015@gmail.com>",
        "Emil Engler <me@emilengler.com>",
    ],
    "emu": [
        "emu <emu@inbox.srvmin.network>",
    ],
    "ENikS": [
        "ENikS <evgeni@eniks.com>",
    ],
    "Enoch Azariah": [
        "enoch <enirox001@gmail.com>",
        "Enoch Azariah <enirox001@gmail.com>",
    ],
    "Epic Curious": [
        "Epic Curious <109078515+epiccurious@users.noreply.github.com>",
    ],
    "epysqyli": [
        "epysqyli <49974367+epysqyli@users.noreply.github.com>",
    ],
    "Eric Hosmer": [
        "Eric Hosmer <EricJ2190@gmail.com>",
    ],
    "Eric Lombrozo": [
        "Eric Lombrozo <elombrozo@gmail.com>",
    ],
    "Eric R. Schulz": [
        "Eric R. Schulz <ersmail@gmail.com>",
    ],
    "Eric S. Bullington": [
        "Eric S. Bullington <eric.s.bullington@gmail.com>",
    ],
    "Eric Scrivner": [
        "Eric Scrivner <eric.t.scrivner@gmail.com>",
    ],
    "Eric Shaw": [
        "Eric Shaw",
        "Eric Shaw <ericshaw.linux@gmail.com>",
        "Eric Shaw Jr <ericshaw.linux@gmail.com>",
        "Eric Shaw <ericshawlinux@users.noreply.github.com>",
        "ericshawlinux <ericshawlinux@users.noreply.github.com>",
    ],
    "Eric Swanson": [
        "Eric Swanson <eswanson@alloscomp.com>",
    ],
    "Erik Arvstedt": [
        "Erik Arvstedt <erik.arvstedt@gmail.com>",
    ],
    "Erik McKelvey": [
        "Erik McKelvey <Erik.McKelvey.is@gmail.com>",
    ],
    "Erik Mossberg": [
        "Erik Mossberg <lingonvecka@gmail.com>",
    ],
    "eriknylund": [
        "eriknylund <erik@daychanged.com>",
    ],
    "Ernest Hemingway": [
        "Ernest Hemingway <coomni120@gmail.com>",
    ],
    "error10": [
        "error10 <error@ioerror.us>",
    ],
    "esneider": [
        "esneider <dariosn@gmail.com>",
    ],
    "espi3": [
        "espi3 <espi3@example.com>",
    ],
    "Esteban Ordano": [
        "Esteban Ordano <eordano@gmail.com>",
    ],
    "Ethan Heilman": [
        "Ethan Heilman",
        "e0 <ethan.r.heilman@gmail.com>",
        "Ethan Heilman <ethan.r.heilman@gmail.com>",
        "Ethan Heilman <Ethan.R.Heilman@gmail.com>",
        "EthanHeilman <ethan.r.heilman@gmail.com>",
        "Ethan Heilman <ethan@geographicslab.org>",
    ],
    "Eugene Siegel": [
        "Crypt-iQ",
        "eugene <elzeigel@gmail.com>",
        "Eugene Siegel <elzeigel@gmail.com>",
        "nsa <elzeigel@gmail.com>",
    ],
    "Eunoia": [
        "Eunoia <33084435+Eunoia1729@users.noreply.github.com>",
    ],
    "Eunovo": [
        "Eunovo <eunovo9@gmail.com>",
        "Novo <eunovo9@gmail.com>",
    ],
    "Eval EXEC": [
        "Eval EXEC <execvy@gmail.com>",
    ],
    "Evan Klitzke": [
        "Evan Klitzke <evan@eklitzke.org>",
    ],
    "Everett Forth": [
        "Everett Forth <everett.forth@gmail.com>",
    ],
    "ezegom": [
        "ezegom <ezegom@bu.edu>",
    ],
    "Fabian H jr.": [
        "Fabian H jr. <fabianherediajr@yahoo.com.mx>",
    ],
    "Fabian Jahr": [
        "Fabian Jahr",
        "fjahr",
        "Fabian Jahr <fabian.jahr@whu.edu>",
        "Fabian Jahr <fjahr@protonmail.com>",
    ],
    "Fabian Raetz": [
        "Fabian Raetz <fabian.raetz@gmail.com>",
    ],
    "Fabrice Drouin": [
        "Fabrice Drouin",
        "sstone",
        "Fabrice Drouin <fabrice@acinq.fr>",
        "Fabrice Drouin <fabrice.drouin@acinq.fr>",
        "sstone <fabrice.drouin@acinq.fr>",
        "sstone <fabrice@acinq.fr>",
    ],
    "Fabrice Fontaine": [
        "Fabrice Fontaine <fontaine.fabrice@gmail.com>",
    ],
    "fcicq": [
        "fcicq <fcicq@fcicq.net>",
    ],
    "fdov": [
        "fdov <fd21@pm.me>",
    ],
    "Federico Bond": [
        "Federico Bond <federicobond@gmail.com>",
    ],
    "Federico Faggiano": [
        "Federico Faggiano <federico.sk@katamail.com>",
    ],
    "Felix Weis": [
        "Felix Weis <mail@felixweis.com>",
    ],
    "Felix Wolfsteller": [
        "Felix Wolfsteller <felix.wolfsteller@gmail.com>",
    ],
    "Ferdinando M. Ametrano": [
        "Ferdinando M. Ametrano",
        "Ferdinando M. Ametrano <fametrano@users.noreply.github.com>",
        "Ferdinando M. Ametrano <ferdinando@ametrano.net>",
    ],
    "Fibonacci747": [
        "Fibonacci747 <albertofibonacci12@gmail.com>",
    ],
    "Filip Gospodinov": [
        "Filip Gospodinov <f@gospodinov.ch>",
    ],
    "fivepiece": [
        "fivepiece <fivepiece@users.noreply.github.com>",
    ],
    "flack": [
        "flack <flack@contentcontrol-berlin.de>",
    ],
    "Flavien Charlon": [
        "Flavien Charlon <flavien@charlon.net>",
    ],
    "Florian Baumgartl": [
        "Florian Baumgartl <florian@baumgartl.net>",
    ],
    "Florian Schmaus": [
        "Florian Schmaus <flo@geekplace.eu>",
    ],
    "Florin": [
        "Florin <florin@libertv.ro>",
    ],
    "flower": [
        "flower <flower@k1024.de>",
    ],
    "Forrest Voight": [
        "Forrest Voight <forrest@forre.st>",
        "Forrest Voight <forrest@forrest-laptop.(none)>",
    ],
    "Fotis Koutoupas": [
        "Fotis Koutoupas <fotis.koutoupas@gmail.com>",
    ],
    "FractalEncrypt": [
        "FractalEncrypt <FractalEncrypt@TimechainArtifacts.com>",
    ],
    "Francesco 'makevoid' Canessa": [
        "Francesco 'makevoid' Canessa <makevoid@gmail.com>",
    ],
    "Francis GASCHET": [
        "Francis GASCHET <fg@numlog.fr>",
    ],
    "Franck Royer": [
        "Franck Royer <franck@coblox.tech>",
    ],
    "frankomosh": [
        "frankomosh <frankomosh197@gmail.com>",
    ],
    "freenancial": [
        "freenancial <freenancial@protonmail.com>",
    ],
    "freewil": [
        "freewil <sean@eternalrise.com>",
    ],
    "fridokus": [
        "fridokus",
        "fridokus <oskar.fridell@gmail.com>",
        "fridokus <raxomukus@gmail.com>",
    ],
    "fsb4000": [
        "fsb4000 <fsb4000@yandex.ru>",
    ],
    "Fu Yong Quah": [
        "Fu Yong Quah",
        "Fu Yong Quah <stanislavwritescode@gmail.com>",
        "fyquah",
        "fyquah <fyquah@protonmail.com>",
    ],
    "fuder.eth": [
        "fuder.eth <vtljgrinkiv@gmail.com>",
    ],
    "Fuzzbawls": [
        "Fuzzbawls <fuzzbawls@gmail.com>",
    ],
    "Gabriel Davidian": [
        "Gabriel Davidian <gabrielius.dav@gmail.com>",
    ],
    "Gabriele Bocchi": [
        "Gabriele Bocchi <40704221+GabrieleBocchi@users.noreply.github.com>",
    ],
    "Gasper Cefarin": [
        "Chuf",
        "Gasper Cefarin",
        "gchuf",
        "42591821+gapeman@users.noreply.github.com",
        "42591821+GChuf@users.noreply.github.com",
        "gasper.cefarin@gmail.com",
        "Chuf <42591821+gapeman@users.noreply.github.com>",
        "Chuf <42591821+GChuf@users.noreply.github.com>",
        "Gasper Cefarin <gasper.cefarin@gmail.com>",
        "gapeman <gasper.cefarin@gmail.com>",
        "gchuf <gasper.cefarin@gmail.com>",
        "GChuf <gasper.cefarin@gmail.com>",
    ],
    "Gastón I. Silva": [
        "Gastón I. Silva <givanse@users.noreply.github.com>",
    ],
    "Gaurav Rana": [
        "Gaurav Rana <bitcoinsSG@gmail.com>",
    ],
    "Gavin Andresen": [
        "Gavin Andresen <gavinandresen@gmail.com>",
        "gavinandresen <gavinandresen@1a98c847-1fd6-4fd8-948a-caf3550aa51b>",
    ],
    "Geoffrey Tsui": [
        "Geoffrey Tsui <tsui.geoffrey@gmail.com>",
    ],
    "Gert-Jaap Glasbergen": [
        "Gert-Jaap Glasbergen <gertjaap@gertjaap.org>",
    ],
    "ghander": [
        "ghander <cen254@gmail.com>",
    ],
    "Giel van Schijndel": [
        "Giel van Schijndel <me@mortis.eu>",
    ],
    "Gillian Chu": [
        "Gillian Chu <gillianchu@Gillians-MacBook-Pro.local>",
    ],
    "GitHub": [
        "GitHub <noreply@github.com>",
    ],
    "Giulio Lombardo": [
        "Giulio Lombardo <giulio.lombardo@gmail.com>",
    ],
    "Giuseppe Mazzotta": [
        "Giuseppe Mazzotta <gdm85@users.noreply.github.com>",
    ],
    "gjs278": [
        "gjs278 <admin@garyshood.com>",
    ],
    "gladoscc": [
        "gladoscc",
        "gladoscc <admin@glados.cc>",
        "gladoscc <elacoin@glados.cc>",
    ],
    "Gleb Naumenko": [
        "Gleb <naumenko.gs@gmail.com>",
        "Gleb Naumenko <naumenko.gs@gmail.com>",
        "User <naumenko.gs@gmail.com>",
    ],
    "Glenn Willen": [
        "Glenn Willen <gwillen@nerdnet.org>",
        "gwillen <gwillen@nerdnet.org>",
    ],
    "globalcitizen": [
        "globalcitizen <walter@pratyeka.org>",
    ],
    "Gloria Wang": [
        "Gloria Wang",
        "glowang <wanggloria21@gmail.com>",
        "Gloria Wang <wanggloria21@gmail.com>",
    ],
    "Gloria Zhao": [
        "Gloria Zhao",
        "glozow",
        "gloriajzhao@gmail.com",
        "gzhao408@berkeley.edu",
        "Gloria Zhao <gloriajzhao@gmail.com>",
        "Gloria Zhao <gzhao408@berkeley.edu>",
        "glozow <gloriajzhao@gmail.com>",
        "glozow <gzhao408@berkeley.edu>",
        "gzhao408 <gzhao408@berkeley.edu>",
        "merge-script <gloriajzhao@gmail.com>",
    ],
    "gnuser": [
        "gnuser <cjay0106@gmail.com>",
    ],
    "Gr0kchain": [
        "Gr0kchain",
        "Gr0kchain <7654306+gr0kchain@users.noreply.github.com>",
        "gr0kchain <gr0kchain@bitcoindev.network>",
    ],
    "Grady Laksmono": [
        "Grady Laksmono <grady@laksmono.com>",
    ],
    "Graham Krizek": [
        "Graham Krizek",
        "Graham Krizek <gkrizek@nodesource.com>",
        "Graham Krizek <graham@krizek.io>",
    ],
    "graingert": [
        "graingert <tagrain@gmail.com>",
    ],
    "GreatSock": [
        "GreatSock <greatsock@protonmail.com>",
    ],
    "Greg Griffith": [
        "Greg Griffith <cryptounitedteam@gmail.com>",
    ],
    "Greg Walker": [
        "Greg Walker <greg@learnmeabitcoin.com>",
    ],
    "Greg Weber": [
        "Greg Weber <gregwebs@users.noreply.github.com>",
    ],
    "Gregory Maxwell": [
        "Gregory Maxwell",
        "gmaxwell",
        "gmaxwell@gmail.com",
        "greg@xiph.org",
        "gmaxwell <gmaxwell@gmail.com>",
        "Gregory Maxwell <gmaxwell@gmail.com>",
        "Gregory Maxwell <greg@xiph.org>",
    ],
    "Gregory Sanders": [
        "Gregory Sanders",
        "instagibbs",
        "gsanders87@gmail.com",
        "BitcoinPRReadingGroup <gsanders.87@gmail.com>",
        "Gregory Sanders <gsanders87@gmail.com>",
        "instagibbs <gsanders87@gmail.com>",
        "Greg Sanders <gsanders87@gmail.com>",
    ],
    "grim-trigger": [
        "grim-trigger <36375872+grim-trigger@users.noreply.github.com>",
    ],
    "grimd34th": [
        "grimd34th <ubpd34th@gmail.com>",
    ],
    "grubles": [
        "grubles <grubles@users.noreply.github.com>",
    ],
    "gruve-p": [
        "gruve-p",
        "gruve-p <gruve-p@users.noreply.github.com>",
        "gruve-p <jackielove4u@hotmail.com>",
        "jackielove4u <jackielove4u@hotmail.com>",
    ],
    "gubatron": [
        "gubatron <gubatron@gmail.com>",
    ],
    "Guido Vranken": [
        "Guido Vranken <guidovranken@gmail.com>",
    ],
    "Guillermo Céspedes Tabárez": [
        "Guillermo Céspedes Tabárez <dev.dertin@gmail.com>",
    ],
    "Guillermo Fernandes": [
        "Guillermo Fernandes <39845628+fernandguil@users.noreply.github.com>",
    ],
    "Gunar C. Gessner": [
        "Gunar C. Gessner <gunargessner@gmail.com>",
        "Gunar Gessner <gunargessner@gmail.com>",
    ],
    "gustavonalle": [
        "gustavonalle <gustavonalle@gmail.com>",
    ],
    "Gutflo": [
        "Gutflo <107882881+klein818@users.noreply.github.com>",
    ],
    "h": [
        "h <harshit_goyal333@outlook.com>",
    ],
    "Haakon Nilsen": [
        "Haakon Nilsen <haakonn@gmail.com>",
    ],
    "hackerrdave": [
        "hackerrdave <davekerrcode@gmail.com>",
    ],
    "HaltingState": [
        "HaltingState <haltingstate@gmail.com>",
    ],
    "Hampus Sjöberg": [
        "Hampus Sjöberg <hampus.sjoberg@gmail.com>",
    ],
    "Han Lin Yap": [
        "Han Lin Yap <codler@gmail.com>",
    ],
    "hanmz": [
        "hanmz <hanmzarsenal@gmail.com>",
    ],
    "Hao Xu": [
        "Hao Xu <hao.xu@linux.dev>",
    ],
    "Haoran Peng": [
        "Haoran Peng <hrhrpeng@outlook.com>",
    ],
    "Haowen Liu": [
        "Haowen Liu <35328328+lunacd@users.noreply.github.com>",
    ],
    "HAOYUatHZ": [
        "HAOYUatHZ <haoyu@protonmail.com>",
    ],
    "Harris Brakmic": [
        "Harris Brakmic",
        "Harris <brakmic@gmail.com>",
        "Harris Brakmic <brakmic@gmail.com>",
    ],
    "Harry Moreno": [
        "Harry Moreno <morenoh149@gmail.com>",
    ],
    "HarryWu": [
        "harry <harrywu@tvunetworks.com>",
        "HarryWu <imharrywu@users.noreply.github.com>",
        "imharrywu <imharrywu@users.noreply.github.com>",
    ],
    "Heath": [
        "Heath <heathmatlock@gmail.com>",
    ],
    "Hector Jusforgues": [
        "Hector Jusforgues <contact@hectorj.net>",
    ],
    "Heebs": [
        "Heebs <hieblmi@gmail.com>",
    ],
    "Hennadii Stepanov": [
        "Hennadii Stepanov",
        "hebasto",
        "32963518+hebasto@users.noreply.github.com",
        "Hennadii Stepanov <32963518+hebasto@users.noreply.github.com>",
        "merge-script <32963518+hebasto@users.noreply.github.com>",
    ],
    "Henrik Jonsson": [
        "Henrik Jonsson <me@hkjn.me>",
    ],
    "Henry Romp": [
        "Henry Romp <151henry151@gmail.com>",
    ],
    "Hernan Marino": [
        "Hernan Marino",
        "Hernan Marino <hernanmarino@protonmail.com>",
        "Hernan Marino <hmarino@gmail.com>",
        "hernanmarino <87907936+hernanmarino@users.noreply.github.com>",
    ],
    "hiago": [
        "hiago <hiago.dutra@gmail.com>",
    ],
    "HiLivin": [
        "HiLivin <magicjakub@gmail.com>",
    ],
    "himynameismartin": [
        "himynameismartin <himynameismartin@gmail.com>",
    ],
    "Hodlinator": [
        "Hodlinator",
        "172445034+hodlinator@users.noreply.github.com",
        "Hodlinator <172445034+hodlinator@users.noreply.github.com>",
        "hodlinator <172445034+hodlinator@users.noreply.github.com>",
    ],
    "hoffman": [
        "hoffman <bettthoffman@gmail.com>",
    ],
    "HostFat": [
        "HostFat <hostfat@gmail.com>",
    ],
    "HouseOfHufflepuff": [
        "HouseOfHufflepuff <ahrens@gmail.com>",
    ],
    "Huang Le": [
        "Huang Le <4tarhl@gmail.com>",
    ],
    "Hugo Nguyen": [
        "Hugo Nguyen <hugh.hn@gmail.com>",
    ],
    "i-am-yuvi": [
        "i-am-yuvi <yuvichh01@gmail.com>",
        "yuvicc <yuvichh01@gmail.com>",
    ],
    "iamcarlos94": [
        "iamcarlos94 <62184065+iamcarlos94@users.noreply.github.com>",
    ],
    "Ian Carroll": [
        "Ian Carroll <him@ian.sh>",
    ],
    "Ian Kelling": [
        "Ian Kelling <ian@iankelling.org>",
    ],
    "Ian T": [
        "Ian T <hello@chainquery.com>",
    ],
    "ianliu": [
        "ianliu <i@yiyangliu.me>",
    ],
    "Igor Bubelov": [
        "Igor Bubelov <igor@bubelov.com>",
    ],
    "Igor Cota": [
        "Igor Cota",
        "Igor Cota <igor@codexapertus.com>",
        "Igor Cota <igor@foundationdevices.com>",
        "Igor Cota <igor@openbook.hr>",
    ],
    "Ikko Ashimine": [
        "Ikko Ashimine <eltociear@gmail.com>",
    ],
    "Indospace.io": [
        "Indospace.io <justin@indospace.io>",
    ],
    "ion-": [
        "ion- <itornaza@gmail.com>",
    ],
    "Irving Ruan": [
        "Irving Ruan <irvingruan@gmail.com>",
    ],
    "ishaanam": [
        "ishaanam <ishaana.misra@gmail.com>",
    ],
    "Isidoro Ghezzi": [
        "Isidoro Ghezzi <isidoro.ghezzi@icloud.com>",
    ],
    "isle2983": [
        "isle2983 <isle2983@yahoo.com>",
    ],
    "Ivan Metlushko": [
        "Ivan Metlushko",
        "Ivan Metlushko <metlushko@gmail.com>",
        "S3RK",
        "S3RK <1466284+S3RK@users.noreply.github.com>",
        "S3RK <1466284+s3rk@users.noreply.github.com>",
    ],
    "Ivan Pustogarov": [
        "Ivan Pustogarov <ivanpustogarov@users.noreply.github.com>",
    ],
    "Ivan Vershigora": [
        "Ivan Vershigora <ivan.vershigora@gmail.com>",
    ],
    "Ivo van der Sangen": [
        "Ivo van der Sangen <ivdsangen@gmail.com>",
    ],
    "J Ross Nicoll": [
        "J Ross Nicoll <jrn@jrn.me.uk>",
        "Ross Nicoll <jrn@jrn.me.uk>",
    ],
    "Jack Grigg": [
        "Jack Grigg <jack@z.cash>",
    ],
    "Jack Mallers": [
        "Jack Mallers",
        "Jack Mallers <jackmallers@Jacks-MacBook-Pro.local>",
        "Jack Mallers <jimmymowschess@gmail.com>",
    ],
    "Jacky C": [
        "Jacky C <jackycjh@users.noreply.github.com>",
    ],
    "Jacob P. Fickes": [
        "Jacob P. Fickes <jacobpfickes@gmail.com>",
    ],
    "Jacob Welsh": [
        "Jacob Welsh <jacob@welshcomputing.com>",
    ],
    "Jadi": [
        "Jadi",
        "jadi@axiros.com",
        "jadijadi@gmail.com",
        "Jadi <jadi@axiros.com>",
        "Jadi <jadijadi@gmail.com>",
    ],
    "Jake Leventhal": [
        "Jake Leventhal <jakeleventhal@me.com>",
    ],
    "Jake Rawsthorne": [
        "Jake Rawsthorne <jake.rawsthorne@couchbase.com>",
    ],
    "Jakob Kramer": [
        "Jakob Kramer <jakob.kramer@gmx.de>",
    ],
    "James Burkle": [
        "James Burkle <james.burkle@gmail.com>",
    ],
    "James Chiang": [
        "James Chiang <james.chiangwu@gmail.com>",
        "JamesC <james.chiangwu@gmail.com>",
    ],
    "James Evans": [
        "James Evans",
        "James Evans <keystrike@gmail.com>",
        "James Evans <keystrike@users.noreply.github.com>",
        "keystrike <keystrike@users.noreply.github.com>",
    ],
    "James Hilliard": [
        "James Hilliard <james.hilliard1@gmail.com>",
    ],
    "James O'Beirne": [
        "James O'Beirne",
        "James O'Beirne <james.obeirne@gmail.com>",
        "James O'Beirne <james.obeirne@pm.me>",
        "James O'Beirne <james@chaincode.com>",
    ],
    "James White": [
        "James White",
        "James White <james@jmwhite.co.uk>",
        "James White <jamesmacwhite@users.noreply.github.com>",
    ],
    "Jameson Lopp": [
        "Jameson Lopp",
        "Jameson Lopp <jameson.lopp@gmail.com>",
        "Jameson Lopp <jameson@bitgo.com>",
        "Jameson Lopp <jameson@team.casa>",
    ],
    "Jan B": [
        "janb84",
        "Jan B <608446+janb84@users.noreply.github.com>",
        "janb84 <608446+janb84@users.noreply.github.com>",
        "janb84 <githubjanb.drainer976@passmail.net>",
    ],
    "Jan Beich": [
        "Jan Beich <jbeich@FreeBSD.org>",
    ],
    "Jan Čapek": [
        "Jan Čapek <jan.capek@braiins.cz>",
    ],
    "Jan Sarenik": [
        "Jan Sarenik <jasan@jasan.tk>",
    ],
    "Janna": [
        "Janna <jg.general@protonmail.com>",
    ],
    "Janne Pulkkinen": [
        "Janne Pulkkinen <jannepulk@gmail.com>",
    ],
    "Janusz Lenar": [
        "Janusz Lenar <malleor@users.noreply.github.com>",
    ],
    "Jarol Rodriguez": [
        "Jarol Rodriguez <jarolrod@tutanota.com>",
        "jarolrod <jarolrod@tutanota.com>",
    ],
    "Jaromil": [
        "Jaromil <jaromil@dyne.org>",
    ],
    "Jarret Dyrbye": [
        "Jarret Dyrbye <jarret.dyrbye@gmail.com>",
    ],
    "JaSK": [
        "JaSK <temp@temp.temp>",
    ],
    "Jason Hester": [
        "Jason Hester <mail@jason-hester.me>",
    ],
    "Jason Lewicki": [
        "Jason Lewicki <lewicki.jason@gmail.com>",
    ],
    "Jay Weisskopf": [
        "Jay Weisskopf <jay@jayschwa.net>",
    ],
    "jayvaliya": [
        "jayvaliya <valiyajay555@gmail.com>",
    ],
    "Jeff Frontz": [
        "Jeff Frontz",
        "Jeff Frontz <jeff.frontz@gmail.com>",
        "Jeff Frontz <jhf@blockstream.io>",
    ],
    "Jeff Garzik": [
        "Jeff Garzik",
        "Jeff Garzik <jeff@bloq.com>",
        "Jeff Garzik <jeff@garzik.org>",
        "Jeff Garzik <jgarzik@bitpay.com>",
        "Jeff Garzik <jgarzik@exmulti.com>",
        "Jeff Garzik <jgarzik@peernova.com>",
        "Jeff Garzik <jgarzik@pobox.com>",
        "Jeff Garzik <jgarzik@redhat.com>",
    ],
    "Jeff Rade": [
        "Jeff Rade <jeffrade@gmail.com>",
    ],
    "Jeff Ruane": [
        "Jeff Ruane <ruane.jb@gmail.com>",
    ],
    "Jeffrey Czyz": [
        "Jeffrey Czyz <jkczyz@gmail.com>",
    ],
    "Jeremiah Buddenhagen": [
        "Jeremiah Buddenhagen <bitspill+git@bitspill.net>",
    ],
    "Jeremy Rand": [
        "Jeremy Rand",
        "Jeremy Rand <jeremyrand@airmail.cc>",
        "Jeremy Rand <jeremyrand@danwin1210.de>",
        "JeremyRand <biolizard89@gmail.com>",
    ],
    "Jeremy Rubin": [
        "Jeremy Rubin",
        "Jeremy Rubin <j@rubin.io>",
        "Jeremy Rubin <jeremy.l.rubin@gmail.com>",
    ],
    "JeremyCrookshank": [
        "JeremyCrookshank <46864828+JeremyCrookshank@users.noreply.github.com>",
    ],
    "Jeroenz0r": [
        "Jeroenz0r <jeroen.marshmallow@gmail.com>",
    ],
    "Jesse Barton": [
        "Jesse Barton <jessebarton95@gmail.com>",
        "jessebarton <jessebarton95@gmail.com>",
    ],
    "Jesse Cohen": [
        "Jesse Cohen <jc@jc.lol>",
    ],
    "jgmorgan": [
        "jgmorgan <james@jgmorgan.com>",
    ],
    "Jiaxing Wang": [
        "Jiaxing Wang <hello.wjx@gmail.com>",
    ],
    "Jim Posen": [
        "Jim Posen",
        "Jim Posen <jim.posen@gmail.com>",
        "Jim Posen <jimpo@coinbase.com>",
    ],
    "Jimmy Song": [
        "Jimmy Song <jaejoon@gmail.com>",
    ],
    "Jiri Jakes": [
        "Jiri Jakes <jiri@jirijakes.com>",
    ],
    "jjz": [
        "jjz <woaf1003@gmail.com>",
    ],
    "JL2035": [
        "JL2035 <jl2035@users.noreply.github.com>",
    ],
    "jloughry": [
        "jloughry <joe.loughry@gmail.com>",
    ],
    "jmacwhyte": [
        "jmacwhyte <keatonatron@gmail.com>",
    ],
    "jmorgan": [
        "jmorgan <jmorgan@onshape.com>",
    ],
    "Joan Karadimov": [
        "Joan Karadimov <joan.karadimov@gmail.com>",
    ],
    "João Barbosa": [
        "João Barbosa",
        "=?UTF-8?q?Jo=C3=A3o=20Barbosa?= <joao.paulo.barbosa@gmail.com>",
        "João Barbosa <joao.paulo.barbosa@gmail.com>",
        "João Barbosa <joao@bitreserve.org>",
        "João Barbosa <joao@uphold.com>",
    ],
    "Joao Fonseca": [
        "Joao Fonseca <jpdf.fonseca@gmail.com>",
    ],
    "JoaoAJMatos": [
        "JoaoAJMatos <joaoafonsoj2004@gmail.com>",
    ],
    "joaonevess": [
        "joaonevess <49655948+joaonevess@users.noreply.github.com>",
    ],
    "Joe Harvell": [
        "Joe Harvell <joe.harvell.x@gmail.com>",
    ],
    "Joel Kaartinen": [
        "Joel Kaartinen <jkaartinen@iki.fi>",
    ],
    "Joel Klabo": [
        "Joel Klabo <joelklabo@gmail.com>",
    ],
    "joemphilips": [
        "joemphilips <joemphilips@gmail.com>",
    ],
    "Joerie de Gram": [
        "Joerie de Gram <j.de.gram@gmail.com>",
    ],
    "Johannes Henninger": [
        "Johannes Henninger <blaubaer@gmail.com>",
    ],
    "Johannes Kanig": [
        "Johannes Kanig <kanigsson@users.noreply.github.com>",
    ],
    "John Bampton": [
        "John Bampton",
        "John Bampton <jbampton@gmail.com>",
        "John Bampton <jbampton@users.noreply.github.com>",
    ],
    "John L. Jegutanis": [
        "John L. Jegutanis <erasmospunk@gmail.com>",
    ],
    "John Maguire": [
        "John Maguire <johnmaguire2013@gmail.com>",
    ],
    "John Moffett": [
        "john-moffett",
        "John Moffett <john.moff@gmail.com>",
    ],
    "John Newbery": [
        "John Newbery",
        "jnewbery",
        "jonnynewbs",
        "john@johnnewbery.com",
        "jonnynewbs@gmail.com",
        "jnewbery <john@johnnewbery.com>",
        "John Newbery <john@johnnewbery.com>",
        "John Newbery <1063656+jnewbery@users.noreply.github.com>",
        "John Newbery <jonnynewbs@gmail.com>",
        "jonnynewbs <jonnynewbs@gmail.com>",
    ],
    "Johnathan Corgan": [
        "Johnathan Corgan <johnathan@corganlabs.com>",
    ],
    "johnlow95": [
        "johnlow95 <38558408+johnlow95@users.noreply.github.com>",
    ],
    "johnny9": [
        "johnny9 <985648+johnny9@users.noreply.github.com>",
    ],
    "Johnson Lau": [
        "Johnson Lau",
        "jl2012 <jl2012@xbt.hk>",
        "Johnson Lau <jl2012@xbt.hk>",
        "Johnson Lau <jl2012@users.noreply.github.com>",
    ],
    "Jon Atack": [
        "Jon Atack",
        "jonatack",
        "Jon Atack <jon@atack.com>",
        "jonatack <jon@atack.com>",
    ],
    "Jon Layton": [
        "Jon Layton",
        "Jon Layton <jon@seequ.com>",
        "Jon Layton <me@jonl.io>",
    ],
    "Jon Lund Steffensen": [
        "Jon Lund Steffensen <jonlst@gmail.com>",
    ],
    "Jonas Nick": [
        "Jonas Nick <jonasd.nick@gmail.com>",
    ],
    "Jonas Schnelli": [
        "Jonas Schnelli",
        "Jonas Schnelli <dev@jonasschnelli.ch>",
        "Jonas Schnelli <jonas.schnelli@include7.ch>",
        "Jonas Schnelli <jonasschnelli@Jonass-MacBook-Pro.local>",
    ],
    "Jonathan Brown": [
        "Jonathan Brown <jbrown@bluedroplet.com>",
    ],
    "Jonathan Cross": [
        "Jonathan Cross <jonathancross@users.noreply.github.com>",
    ],
    "Jonathan Duke Leto": [
        "Jonathan \"Duke\" Leto <jonathan@leto.net>",
    ],
    "Jonathan Schoeller": [
        "Jonathan Schoeller <jonathan.schoeller@rea-group.com>",
    ],
    "Joonmo Yang": [
        "Joonmo Yang <dev@remagpie.com>",
    ],
    "Jordan Baczuk": [
        "Jordan Baczuk",
        "Jordan Baczuk <Jordan.Baczuk@gmail.com>",
        "Jordan Baczuk <jordan.baczuk@gmail.com>",
    ],
    "Jordan Lewis": [
        "Jordan Lewis <jordanthelewis@gmail.com>",
    ],
    "Jorge Timón": [
        "jtimon",
        "Jorge Timón <jtimon@jtimon.cc>",
        "jtimon <jtimon@jtimon.cc>",
        "jtimon <jtimon@blockstream.io>",
        "jtimon <jtimon@monetize.io>",
    ],
    "Jörn Röder": [
        "Jörn Röder <kontakt@joernroeder.de>",
    ],
    "Josh Doman": [
        "Josh Doman <joshsdoman@gmail.com>",
    ],
    "Josh Hartshorn": [
        "Josh Hartshorn <joshhartshorn1021@gmail.com>",
    ],
    "Josh Lehan": [
        "Josh Lehan <krellan@krellan.net>",
    ],
    "Josh Triplett": [
        "Josh Triplett <josh@joshtriplett.org>",
    ],
    "joshr": [
        "joshr <joshr@joshr.com>",
    ],
    "Joshua Kelly": [
        "jdjkelly@gmail.com <jdjkelly@gmail.com>",
        "Joshua Kelly <jdjkelly@gmail.com>",
    ],
    "Josiah Baker": [
        "Josiah Baker <josibake@protonmail.com>",
        "josibake <josibake@protonmail.com>",
    ],
    "Josu Goñi": [
        "Josu Goñi <josu_z@hotmail.com>",
    ],
    "jrakibi": [
        "jrakibi <j.errakibi@gmail.com>",
    ],
    "Juan Pablo Civile": [
        "Juan Pablo Civile <elementohb@gmail.com>",
    ],
    "Julian Fleischer": [
        "Julian Fleischer",
        "Julian Fleischer <julian@thirdhash.com>",
        "Julian Fleischer <scravy@users.noreply.github.com>",
        "Julian Fleischer <tirednesscankill@warhog.net>",
    ],
    "Julian Haight": [
        "Julian Haight <github@a.julianhaight.com>",
    ],
    "Julian Langschaedel": [
        "Julian Langschaedel <meta.rb@gmail.com>",
    ],
    "Julian Yap": [
        "Julian Yap <jyap808@users.noreply.github.com>",
    ],
    "junderw": [
        "junderw <junderwood@bitcoinbank.co.jp>",
    ],
    "jurraca": [
        "jurraca <julienu@pm.me>",
    ],
    "Justin Camarena": [
        "Justin Camarena <justin121994@gmail.com>",
    ],
    "Justin Dhillon": [
        "Justin Dhillon <justin.singh.dhillon@gmail.com>",
    ],
    "Justin Litchfield": [
        "Justin Litchfield <litch@me.com>",
    ],
    "Justin Moon": [
        "Justin Moon <mail@justinmoon.com>",
    ],
    "Justin Turner Arthur": [
        "Justin Turner Arthur <justinarthur@gmail.com>",
    ],
    "justmoon": [
        "justmoon <justmoon@members.fsf.org>",
    ],
    "Kamil Domanski": [
        "Kamil Domanski <kdomanski@kdemail.net>",
    ],
    "Kangmo": [
        "Kangmo <kangmo@nanolat.com>",
    ],
    "kanon": [
        "kanon <60179867+decryp2kanon@users.noreply.github.com>",
    ],
    "Karel Bilek": [
        "Karel Bilek",
        "Karel Bilek <kb@karelbilek.com>",
        "Karel Bílek <kb@karelbilek.com>",
    ],
    "Karl-Johan Alm": [
        "Karl-Johan Alm",
        "Kalle Alm <kalle.alm@gmail.com>",
        "kallewoof <kalle.alm@gmail.com>",
        "Karl-Johan Alm <kalle.alm@gmail.com>",
        "Karl-Johan Alm <karljohan-alm@garage.co.jp>",
    ],
    "Kashif Smith": [
        "Kashif Smith <1489460+kashifs@users.noreply.github.com>",
    ],
    "Kate Salazar": [
        "katesalazar",
        "Kate Salazar <52637275+katesalazar@users.noreply.github.com>",
        "katesalazar <52637275+katesalazar@users.noreply.github.com>",
        "katesalazar <mercedes.catherine.salazar@gmail.com>",
    ],
    "Kay": [
        "Kay <kehiiiiya@gmail.com>",
    ],
    "Kaz Wesley": [
        "Kaz Wesley",
        "Kaz Wesley <kaz@lambdaverse.org>",
        "Kaz Wesley <keziahw@gmail.com>",
        "kazcw <keziahw@gmail.com>",
    ],
    "kdmukai": [
        "kdmukai <kdmukai@gmail.com>",
    ],
    "keepkeyjon": [
        "keepkeyjon",
        "keepkeyjon <35975617+keepkeyjon@users.noreply.github.com>",
        "keepkeyjon <jon@keepkey.com>",
    ],
    "Kefkius": [
        "Kefkius <kefkius@mail.com>",
    ],
    "keneanung": [
        "keneanung <keneanung@googlemail.com>",
    ],
    "Kennan Mell": [
        "Kennan Mell <kmell96@gmail.com>",
    ],
    "Kevin Cooper": [
        "Kevin Cooper <k.coopr@gmail.com>",
    ],
    "Kevin Musgrave": [
        "Kevin Musgrave <tkm45@cornell.edu>",
        "KevinMusgrave <tkm45@cornell.edu>",
        "TakeshiMusgrave <tkm45@cornell.edu>",
    ],
    "Kevin Pan": [
        "kevin <bit.kevin@gmail.com>",
        "Kevin Pan <bit.kevin@gmail.com>",
    ],
    "kevkevin": [
        "kevkevin <oapallikunnel@gmail.com>",
        "kevkevinpal <oapallikunnel@gmail.com>",
    ],
    "Kewde": [
        "Kewde <code@shadowproject.io>",
    ],
    "Khalahan": [
        "Khalahan <khal@bitcoin-contact.org>",
    ],
    "KibbledJiveElkZoo": [
        "KibbledJiveElkZoo <KibbledJiveElkZoo@GMail.com>",
    ],
    "kilavvy": [
        "kilavvy <140459108+kilavvy@users.noreply.github.com>",
    ],
    "Kiminuo": [
        "Kiminuo <kiminuo@protonmail.com>",
    ],
    "Kirill Fomichev": [
        "Kirill Fomichev <fanatid@ya.ru>",
    ],
    "kirit93": [
        "kirit93 <kirit.thadaka@gmail.com>",
    ],
    "kirkalx": [
        "kirkalx <kirkalx@yahoo.co.nz>",
    ],
    "Kittywhiskers Van Gogh": [
        "Kittywhiskers Van Gogh <63189531+kittywhiskers@users.noreply.github.com>",
    ],
    "kjj2": [
        "kjj2 <github@jerviss.org>",
    ],
    "klemens": [
        "klemens <ka7@github.com>",
    ],
    "Klement Tan": [
        "klementtan",
        "Klement Tan <klementtan@gmail.com>",
        "klementtan <klementtan@gmail.com>",
        "klementtan <klement.tan@ninjavan.co>",
    ],
    "kobake": [
        "kobake <kobake@users.sourceforge.net>",
    ],
    "kodslav": [
        "kodslav <kodslav@home.local>",
    ],
    "Koki Takahashi": [
        "Koki Takahashi",
        "Koki Takahashi <k.takahashi@sonyged.com>",
        "Koki Takahashi <Koki.Takahashi@jp.sony.com>",
    ],
    "Kolby ML": [
        "Kolby ML <31669092+KolbyML@users.noreply.github.com>",
        "Kolby Moroz Liebl <31669092+KolbyML@users.noreply.github.com>",
    ],
    "Konstantin Akimov": [
        "Konstantin Akimov <knstqq@gmail.com>",
    ],
    "Kosta Zertsekel": [
        "Kosta Zertsekel <zertsekel@gmail.com>",
    ],
    "Kostiantyn Stepaniuk": [
        "Kostiantyn Stepaniuk <kostya.stepanyuk@gmail.com>",
    ],
    "Kristaps Kaupe": [
        "kristapsk",
        "Kristaps Kaupe <kristaps@blogiem.lv>",
    ],
    "Kristian Kramer": [
        "Kristian Kramer <kristian@beyonddata.llc>",
    ],
    "Krzysztof Jurewicz": [
        "Krzysztof Jurewicz <krzysztof.jurewicz@gmail.com>",
    ],
    "Kuro": [
        "Kuro <kuroguo@outlook.com>",
    ],
    "kwaaak": [
        "kwaaak <kwaaak@gmail.com>",
    ],
    "Kyle Honeycutt": [
        "Kyle <coinables@gmail.com>",
        "Kyle Honeycutt <coinables@gmail.com>",
    ],
    "Kyuntae Ethan Kim": [
        "Kyuntae Ethan Kim <ethan.kyuntae.kim@gmail.com>",
    ],
    "L0la L33tz": [
        "L0la L33tz <L0l4L33tz@proton.me>",
    ],
    "Lake Denman": [
        "Lake Denman <lake@lakedenman.com>",
    ],
    "langerhans": [
        "langerhans <max.keller@gmx.com>",
    ],
    "Larry Gilbert": [
        "Larry Gilbert <larry@l2g.to>",
    ],
    "Larry Ruane": [
        "Larry Ruane <larryruane@gmail.com>",
    ],
    "Lars Rasmusson": [
        "Lars Rasmusson <Lars.Rasmusson@sics.se>",
    ],
    "Laszlo Hanyecz": [
        "laszloh",
        "laszloh <laszloh@1a98c847-1fd6-4fd8-948a-caf3550aa51b>",
    ],
    "Lauda": [
        "Lauda <lauda.m@protonmail.ch>",
        "laudaa <lauda.m@protonmail.ch>",
    ],
    "Lawrence Nahum": [
        "Lawrence Nahum <lawrence@greenaddress.it>",
    ],
    "leijurv": [
        "leijurv <leijurv@gmail.com>",
    ],
    "Lenny Maiorani": [
        "Lenny Maiorani <lenny@colorado.edu>",
    ],
    "Leonardo Araujo": [
        "Leonardo Araujo <leonardo.aa88@gmail.com>",
    ],
    "Leonardo Lazzaro": [
        "Leonardo Lazzaro <llazzaro@dc.uba.ar>",
    ],
    "leopardracer": [
        "leopardracer <136604165+leopardracer@users.noreply.github.com>",
    ],
    "Leviathn": [
        "Leviathn <johnny@blockstream.io>",
    ],
    "lewuathe": [
        "lewuathe <lewuathe@me.com>",
    ],
    "Linrono": [
        "Linrono",
        "24950563+Linrono@users.noreply.github.com",
        "linrono@gmail.com",
        "Linrono <24950563+Linrono@users.noreply.github.com>",
        "Linrono <linrono@gmail.com>",
    ],
    "Liran Cohen": [
        "Liran Cohen <c.liran.c@gmail.com>",
    ],
    "lisa neigut": [
        "lisa neigut <niftynei@gmail.com>",
    ],
    "liuyujun": [
        "liuyujun <liuyujun@fingera.cn>",
    ],
    "lizhi": [
        "lizhi <cqtenq9@gmail.com>",
    ],
    "Loganaden Velvindron": [
        "Loganaden Velvindron <logan@hackers.mu>",
    ],
    "LongShao007": [
        "LongShao007 <007longshao@gmail.com>",
    ],
    "Lőrinc": [
        "Lőrinc",
        "Lorinc",
        "l0rinc",
        "paplorinc",
        "pap.lorinc@gmail.com",
        "l0rinc <l0rinc@users.noreply.github.com>",
        "l0rinc <pap.lorinc@gmail.com>",
        "Lőrinc <lorinc.pap@gmail.com>",
        "Lőrinc <pap.lorinc@gmail.com>",
    ],
    "Lowell Manners": [
        "lmanners <lowellmanners@gmail.com>",
        "Lowell Manners <lowellmanners@gmail.com>",
    ],
    "lpescher": [
        "lpescher <lukas_078@yahoo.ca>",
    ],
    "lsilva01": [
        "lsilva01",
        "lsilva01 <84432093+lsilva01@users.noreply.github.com>",
        "lsilva01 <lsilva01@protonmail.com>",
    ],
    "Luca Venturini": [
        "Luca Venturini <luca@yepa.com>",
    ],
    "Lucas Betschart": [
        "Lucas Betschart <lucasbetschart@gmail.com>",
    ],
    "Lucas Ontivero": [
        "lontivero <lucasontivero@gmail.com>",
        "Lucas Ontivero <lucasontivero@gmail.com>",
    ],
    "lucash-dev": [
        "lucash-dev <lucash.dev@gmail.com>",
        "lucash.dev@gmail.com <lucash.dev@gmail.com>",
    ],
    "luciana": [
        "luciana <lucianadacostamarques@gmail.com>",
    ],
    "Luis Schwab": [
        "Luis Schwab <luisschwab@protonmail.com>",
    ],
    "Luke": [
        "Luke <lukem512@users.noreply.github.com>",
    ],
    "Luke Dashjr": [
        "Luke Dashjr",
        "luke-jr",
        "luke+github_public@dashjr.org",
        "luke-jr+git@utopios.org",
        "luke_github1@dashjr.org",
        "Luke Dashjr <luke+github_public@dashjr.org>",
        "Luke Dashjr <luke-jr+git@utopios.org>",
        "Luke Dashjr <luke_github1@dashjr.org>",
    ],
    "Luke Mlsna": [
        "Luke Mlsna <luke@mlsna.net>",
    ],
    "lutangar": [
        "lutangar <johan.dufour@gmail.com>",
    ],
    "Luv Khemani": [
        "Luv Khemani <luvb@hotmail.com>",
    ],
    "Maayan Keshet": [
        "Maayan Keshet <maayan@maayank.com>",
    ],
    "Maciej S. Szmigiero": [
        "Maciej S. Szmigiero <mail@maciej.szmigiero.name>",
    ],
    "Mackain": [
        "Mackain <markus.j.lindgren@gmail.com>",
    ],
    "maiiz": [
        "maiiz <maiiz@users.noreply.github.com>",
    ],
    "malevolent": [
        "malevolent <malevolentbtc@gmail.com>",
    ],
    "mammix2": [
        "mammix2 <mammix2@hotmail.com>",
    ],
    "Manuel Araoz": [
        "Manuel Araoz <manuelaraoz@gmail.com>",
    ],
    "MapleLaker": [
        "MapleLaker <31602441+MapleLaker@users.noreply.github.com>",
    ],
    "Mara van der Laan": [
        "Mara van der Laan",
        "Wladimir J. van der Laan",
        "Wladimir van der Laan",
        "laanwj",
        "laanwj <126646+laanwj@users.noreply.github.com>",
        "Mara van der Laan <126646+laanwj@users.noreply.github.com>",
        "Wladimir J. van der Laan <laanwj@gmail.com>",
        "Wladimir J. van der Laan <laanwj@protonmail.com>",
        "Wladimir van der Laan <laanwj@gmail.com>",
        "W. J. van der Laan <laanwj@protonmail.com>",
    ],
    "marcaiaf": [
        "marcaiaf <mmachicao@m19r.de>",
    ],
    "Marcel Krüger": [
        "Marcel Krüger <zauguin@gmail.com>",
    ],
    "Marcin Jachymiak": [
        "Marcin Jachymiak",
        "Marcin Jachymiak <marcin.jachymiak1@gmail.com>",
        "Marcin Jachymiak <marcin@bitcoinops.org>",
        "Marcin Jachymiak <marcinja@mit.edu>",
    ],
    "marco": [
        "marco <marleo23@proton.me>",
        "marcofleon <marleo23@proton.me>",
    ],
    "Marco Agner": [
        "Marco Agner",
        "marcoagner <marco@agner.io>",
        "Marco Agner <marco@agner.io>",
    ],
    "Marco Falke": [
        "Marco Falke",
        "MarcoFalke",
        "maflcko",
        "MacrabFalke",
        "MacroFake",
        "falke.marco@gmail.com",
        "MarcoFalke@gmail.com",
        "6399679+MarcoFalke@users.noreply.github.com",
        "6399679+maflcko@users.noreply.github.com",
        "*~=`'#}+{/-|&$^_@721217.xyz",
        "*~=`'#}+{/-|&status@721217.xyz",
        "MarcoFalke falke.marco@gmail.com",
    ],
    "Marcos Mayorga": [
        "Marcos Mayorga",
        "Marcos Mayorga <marcos@ncrypt.com>",
        "Marcos Mayorga <mm@mm-studios.com>",
    ],
    "marcuswin": [
        "marcuswin <46599751+marcuswin@users.noreply.github.com>",
    ],
    "Marijn Stollenga": [
        "Marijn Stollenga <m.stollenga@gmail.com>",
    ],
    "Mario Dian": [
        "Mario Dian <mariodian@gmail.com>",
    ],
    "Marius Hanne": [
        "Marius Hanne <marius.hanne@sourceagency.org>",
    ],
    "Marius Kjærstad": [
        "sandakersmann",
        "Marius Kjærstad <sandakersmann@users.noreply.github.com>",
        "sandakersmann <sandakersmann@users.noreply.github.com>",
        "sandakersmann <mkjaerstad@yahoo.no>",
    ],
    "mark": [
        "mark <mark@shotgunsoftware.com>",
    ],
    "Mark Friedenbach": [
        "Mark Friedenbach",
        "Mark Friedenbach <mark@blockstream.io>",
        "Mark Friedenbach <mark@friedenbach.org>",
        "Mark Friedenbach <mark@monetize.io>",
    ],
    "Mark Murch Erhardt": [
        "Murch",
        "Mark Murch Erhardt",
        "AlSzacrel",
        "murchandamus",
        "murch@murch.one",
        "uwblp@student.kit.edu",
        "alszacrel@web.de",
        "mark@bitgo.com",
        "Mark Erhardt <mark@bitgo.com>",
    ],
    "Mark Tyneway": [
        "Mark Tyneway <mark@purse.io>",
    ],
    "Marko Bencun": [
        "Marko Bencun <marko.bencun@monetas.net>",
    ],
    "Marnix Croes": [
        "Marnix Croes",
        "Marnix <93143998+MarnixCroes@users.noreply.github.com>",
        "MarnixCroes <93143998+MarnixCroes@users.noreply.github.com>",
    ],
    "Martin Ankerl": [
        "Martin Ankerl",
        "martinus",
        "Martin Ankerl <Martin.Ankerl@gmail.com>",
        "Martin Ankerl <martin.ankerl@gmail.com>",
        "Martin Leitner-Ankerl <martin.ankerl@gmail.com>",
        "Martin Leitner-Ankerl <martin.leitner-ankerl@dynatrace.com>",
    ],
    "Martin Erlandsson": [
        "Martin Erlandsson",
        "Martin Erlandsson <martin@megabit.se>",
        "merland <martin@megabit.se>",
        "Martin Erlandsson <merland@users.noreply.github.com>",
    ],
    "Martin Saposnic": [
        "Martin Saposnic <martinsaposnic@gmail.com>",
    ],
    "Martin Zumsande": [
        "Martin Zumsande <mzumsande@gmail.com>",
        "mzumsande <mzumsande@gmail.com>",
    ],
    "Martti Malmi": [
        "sirius-m",
        "sirius-m <sirius-m@1a98c847-1fd6-4fd8-948a-caf3550aa51b>",
    ],
    "Marty Jones": [
        "Marty Jones <murtin.jones@gmail.com>",
    ],
    "Masahiko Hyuga": [
        "Masahiko Hyuga <mail@mhyuga.jp>",
    ],
    "maskoficarus": [
        "maskoficarus",
        "maskoficarus <12724368+maskoficarus@users.noreply.github.com>",
        "maskoficarus <bitcoin@maskoficarus.com>",
    ],
    "Mason Simon": [
        "Mason Simon <masonsimon@gmail.com>",
    ],
    "Mathy Vanvoorden": [
        "Mathy Vanvoorden <mathy@vanvoorden.be>",
    ],
    "Matias Furszyfer": [
        "Matias Furszyfer",
        "furszy <matiasfurszyfer@protonmail.com>",
        "Matias Furszyfer <matiasfurszyfer@protonmail.com>",
        "furszy <mfurszy@protonmail.com>",
        "Matias Furszyfer <mfurszy@protonmail.com>",
    ],
    "Matt": [
        "Matt <sirmatt@ksu.edu>",
    ],
    "Matt Bogosian": [
        "Matt Bogosian <mtb19@columbia.edu>",
        "Matthew Bogosian <mtb19@columbia.edu>",
    ],
    "Matt Clough": [
        "Matt Clough <Matt.clough@pm.me>",
    ],
    "Matt Corallo": [
        "Matt Corallo",
        "BlueMatt",
        "git@bluematt.me",
        "matt@bluematt.me",
        "matt@mattcorallo.com",
        "Matt Corallo (laptop - only while traveling) <matt@mattcorallo.com>",
        "Matt Corallo <matt@mattcorallo.com>",
        "Matt Corallo <git@bluematt.me>",
        "Matt Corallo <matt@bluematt.me>",
    ],
    "Matt Giuca": [
        "Matt Giuca <matt.giuca@gmail.com>",
    ],
    "Matt Quinn": [
        "Matt Quinn <matt@mattjquinn.com>",
    ],
    "Matt Ward": [
        "dannmat",
        "mattwardiom",
        "dannmat <mattwardiom>",
        "Matt Ward <mattwardiom@gmail.com>",
    ],
    "Matt Whitlock": [
        "Matt Whitlock <bitcoin@mattwhitlock.name>",
    ],
    "Matteo Sumberaz": [
        "Matteo Sumberaz <gnappoms@gmail.com>",
    ],
    "Matthew English": [
        "matthias <s.matthew.english@gmail.com>",
        "Matthew English <s-matthew-english@users.noreply.github.com>",
        "S. Matthew English <s-matthew-english@users.noreply.github.com>",
    ],
    "Matthew King": [
        "Matthew King <chohag@jtan.com>",
    ],
    "Matthew Zipkin": [
        "Matthew Zipkin",
        "pinheadmz",
        "Matthew.Zipkin@gmail.com",
        "pinheadmz@gmail.com",
        "pinheadmz@pm.me",
        "Matthew Zipkin <Matthew.Zipkin@gmail.com>",
        "Matthew Zipkin <pinheadmz@gmail.com>",
        "Matthew Zipkin <pinheadmz@pm.me>",
    ],
    "matthias": [
        "matthias <h1395010@connect.hku.hk>",
    ],
    "Matthias Grundmann": [
        "Matthias Grundmann <matthias@glasmail.de>",
    ],
    "Max Edwards": [
        "Max Edwards",
        "m3dwards",
        "Max Edwards <me@maxedwards.me>",
        "Max Edwards <youwontforgetthis@gmail.com>",
    ],
    "Max Kaplan": [
        "Max Kaplan <kaplanmaxe3@gmail.com>",
    ],
    "mb300sd": [
        "mb300sd",
        "mb300sd <mb300sd@git>",
        "mb300sd <mb300sd@github>",
    ],
    "Mccalabrese": [
        "Mccalabrese <nw.calabrese@gmail.com>",
    ],
    "Meeh": [
        "Meeh <meeh@sigterm.no>",
    ],
    "Micha": [
        "Micha",
        "Michagogo",
        "michagogo@server.fake",
        "Michagogo@users.noreply.github.com",
        "Micha <michagogo@server.fake>",
        "Michagogo <michagogo@server.fake>",
        "Micha <Michagogo@users.noreply.github.com>",
        "Michagogo <Michagogo@users.noreply.github.com>",
    ],
    "Michael Bauer": [
        "Michael Bauer <michael@m-bauer.org>",
    ],
    "Michael Bemmerl": [
        "Michael Bemmerl <mail@mx-server.de>",
    ],
    "Michael Chrostowski": [
        "Michael Chrostowski <michael.chrostowski@gmail.com>",
    ],
    "Michael Dietz": [
        "Michael Dietz",
        "Michael Dietz <michael.dietz@waya.ai>",
        "Michael Dietz <michaeldietz@Michaels-MacBook-Air.local>",
    ],
    "Michael Folkson": [
        "Michael Folkson <michaelfolkson@gmail.com>",
    ],
    "Michael Ford": [
        "Michael Ford",
        "fanquake",
        "fanquake@gmail.com",
        "merge-script <fanquake@gmail.com>",
        "fanquake <fanquake@gmail.com>",
        "Fordy <fanquake@gmail.com>",
        "Michael <fanquake@gmail.com>",
        "Michael <fanquake@users.noreply.github.com>",
        "Michael Ford <fanquake@gmail.com>",
        "Michael Ford <fanquake@users.noreply.github.com>",
    ],
    "Michael Goldstein": [
        "Michael Goldstein <michael@bitstein.org>",
    ],
    "Michael Hendricks": [
        "Michael Hendricks <michael@ndrix.org>",
    ],
    "Michael Polzer": [
        "Michael Polzer <hashrate@pm-tech.at>",
    ],
    "Michael Rotarius": [
        "Michael Rotarius <michael-rotarius@rotamedia.de>",
    ],
    "Michael Tidwell": [
        "Michael Tidwell <tidwell@zebedee.io>",
    ],
    "Michal Zima": [
        "Michal Zima <xhire@mujmalysvet.cz>",
        "xHire <xhire@mujmalysvet.cz>",
    ],
    "Michał Zabielski": [
        "Michał Zabielski <zabielski.michal@gmail.com>",
    ],
    "Micky Yun Chan": [
        "Micky Yun Chan <michan@redhat.com>",
    ],
    "Midnight Magic": [
        "Midnight Magic",
        "Midnight Magic <midnightmagic@example.com>",
        "Midnight Magic <midnightmagic@users.noreply.github.com>",
    ],
    "Miguel Herranz": [
        "Miguel Herranz <miguel@ipglider.org>",
    ],
    "Mikael Wikman": [
        "Mikael Wikman <mikael@swedcontent.com>",
    ],
    "Mike Cassano": [
        "Mike Cassano <mcassano@gmail.com>",
    ],
    "Mike Hearn": [
        "Mike Hearn",
        "Mike Hearn <hearn@google.com>",
        "Mike Hearn <mike@plan99.net>",
        "Mike Hearn <mike@riker.plan99.net>",
    ],
    "Mike Schmidt": [
        "Mike Schmidt <schmidty@gmail.com>",
    ],
    "Mike van Rossum": [
        "Mike van Rossum <mike@mikevanrossum.nl>",
    ],
    "Mikerah": [
        "Mikerah <mikerah14@gmail.com>",
    ],
    "Miles Liu": [
        "Miles Liu <miles@bung.cc>",
    ],
    "Misbakh-Soloviev Vadim A": [
        "Misbakh-Soloviev Vadim A <mva@mva.name>",
    ],
    "Mitchell Cash": [
        "Mitchell Cash",
        "Mitchell Cash <mitchell.cash@gmail.com>",
        "Mitchell Cash <mitchell@fastmail.com.au>",
        "Mitchell Cash <mitchell@mitchellcash.com>",
    ],
    "MIZUTA Takeshi": [
        "MIZUTA Takeshi <mizuta.takeshi@fujitsu.com>",
    ],
    "monlovesmango": [
        "monlovesmango <monlovesmango@users.noreply.github.com>",
    ],
    "mrbandrews": [
        "mrbandrews <bandrewsny@gmail.com>",
    ],
    "mruddy": [
        "mruddy",
        "mruddy <6440430+mruddy@users.noreply.github.com>",
        "mruddy <mruddy@users.noreply.github.com>",
    ],
    "mryandao": [
        "mryandao <mryandao@tutanota.com>",
    ],
    "Murray Nesbitt": [
        "Murray Nesbitt <github@nesbitt.ca>",
        "murrayn <github@nesbitt.ca>",
    ],
    "Musa Haruna": [
        "Musa Haruna <hmusa3962@gmail.com>",
    ],
    "Mustafa": [
        "Mustafa <mus@musalbas.com>",
    ],
    "mutatrum": [
        "mutatrum <mutatrum@gmail.com>",
    ],
    "muxator": [
        "muxator <antonio.muci@bancaditalia.it>",
    ],
    "Nadav Ivgi": [
        "Nadav Ivgi <nadav@shesek.info>",
    ],
    "naiyoma": [
        "naiyoma <lankas.aurelia@gmail.com>",
    ],
    "naiza": [
        "naiza <naiza@iitk.ac.in>",
    ],
    "nanlour": [
        "nanlour <tjuwrr@gmail.com>",
    ],
    "Nathan Garabedian": [
        "Nathan Garabedian <ngara23@gmail.com>",
    ],
    "Nathan Marley": [
        "Nathan Marley <nathan.marley@gmail.com>",
    ],
    "Nathaniel Mahieu": [
        "Nathaniel Mahieu <nate@mahie.us>",
    ],
    "Neha Narula": [
        "Neha Narula <narula@gmail.com>",
    ],
    "Nelson Galdeman": [
        "Nelson Galdeman <nelsongaldeman@gmail.com>",
    ],
    "nervana21": [
        "nervana21 <205626986+nervana21@users.noreply.github.com>",
    ],
    "Nick Bosma": [
        "Nick Bosma <nick.bosma@gmail.com>",
    ],
    "Nick Vercammen": [
        "Nick Vercammen <nvercamm@users.noreply.github.com>",
    ],
    "Nick Zhavoronkov": [
        "Nick <nikzhavoronkov@gmail.com>",
        "Nick Zhavoronkov <nikzhavoronkov@gmail.com>",
    ],
    "Nicola Leonardo Susca": [
        "Nicola Leonardo Susca <nicolaleonardo.susca@gmail.com>",
        "NicolaLS",
        "NicolaLS <sus.nym@proton.me>",
    ],
    "Nicolas Benoit": [
        "Nicolas Benoit <nbenoit@tuxfamily.org>",
    ],
    "Nicolas DORIER": [
        "Nicolas DORIER",
        "Nicolas DORIER <nicolas.dorier@gmail.com>",
        "Nicolas Dorier <nicolas.dorier@gmail.com>",
        "nicolas.dorier <nicolas.dorier@gmail.com>",
        "NicolasDorier <nicolas.dorier@gmail.com>",
    ],
    "Nicolas Thumann": [
        "Nicolas Thumann <me@n-thumann.de>",
        "nthumann <me@n-thumann.de>",
    ],
    "nijynot": [
        "nijynot <nijynot@gmail.com>",
    ],
    "NikhilBartwal": [
        "NikhilBartwal <nikhilbartwal1234@gmail.com>",
    ],
    "Niklas Gogge": [
        "Niklas Gogge",
        "dergoegge <n.goeggi@gmail.com>",
        "Niklas Gogge <n.goeggi@gmail.com>",
        "Niklas Gögge <n.goeggi@gmail.com>",
    ],
    "Nikodemas Tuckus": [
        "Nikodemas Tuckus <ntuckus@gmail.com>",
    ],
    "Nikolay Mitev": [
        "face <face@hmel.org>",
        "Nikolay Mitev <face@hmel.org>",
    ],
    "Nils Loewen": [
        "Nils Loewen <nilswloewen@gmail.com>",
    ],
    "Nils Schneider": [
        "Nils Schneider",
        "Nils Schneider <nils.schneider@gmail.com>",
        "Nils Schneider <nils@nilsschneider.net>",
    ],
    "Nima Yazdanmehr": [
        "Nima Yazdanmehr <yazdanmehr@protonmail.com>",
    ],
    "nkostoulas": [
        "nkostoulas <nkostoulas@gmail.com>",
    ],
    "node": [
        "node <node@nodes-Virtual-Machine.local>",
    ],
    "node01": [
        "node01 <mbildwic@protonmail.com>",
    ],
    "Noel Tiernan": [
        "Noel Tiernan <tiernolan@gmail.com>",
    ],
    "nomnombtc": [
        "nomnombtc <mastergizmo@arcor.de>",
    ],
    "ns-xvrn": [
        "ns-xvrn <ns@xvrn.tech>",
    ],
    "ntrgn": [
        "ntrgn <ntrgnt@gmail.com>",
    ],
    "NullFunctor": [
        "NullFunctor <support@bitcoinv.org>",
    ],
    "ojab": [
        "ojab <ojab@ojab.ru>",
    ],
    "okayplanet": [
        "okayplanet <tyrick@gmail.com>",
    ],
    "olalonde": [
        "olalonde <olalonde@gmail.com>",
    ],
    "Oliver Gugger": [
        "Oliver Gugger <gugger@gmail.com>",
    ],
    "Olivier Langlois": [
        "Olivier Langlois <olivier@olivierlanglois.net>",
    ],
    "omahs": [
        "omahs <73983677+omahs@users.noreply.github.com>",
    ],
    "optout": [
        "optout",
        "optout <13562139+optout21@users.noreply.github.com>",
        "optout21 <13562139+optout21@users.noreply.github.com>",
        "optout <optout@nostrplebs.com>",
    ],
    "orient": [
        "orient <orientye@users.noreply.github.com>",
    ],
    "Oskar Mendel": [
        "BrokenProgrammer <brokenprogrammer@gmail.com>",
        "Oskar Mendel <brokenprogrammer@gmail.com>",
    ],
    "osmosis": [
        "osmosis <stevenwagner@gmail.com>",
    ],
    "Otto Allmendinger": [
        "Otto Allmendinger <otto.allmendinger@gmail.com>",
    ],
    "ovdeathiam": [
        "ovdeathiam <krystian.maksymowicz@gmail.com>",
    ],
    "OverlordQ": [
        "OverlordQ <overlordq@gmail.com>",
    ],
    "Pablo Fernandez": [
        "Pablo Fernandez <fernandezpablo85@gmail.com>",
    ],
    "Pablo Greco": [
        "Pablo Greco <psgreco@gmail.com>",
    ],
    "pablomartin4btc": [
        "pablomartin4btc",
        "pablomartin4btc <110166421+pablomartin4btc@users.noreply.github.com>",
        "pablomartin4btc <pablomartin4btc@gmail.com>",
    ],
    "pad": [
        "pad <pad@maitrebitcoin.com>",
    ],
    "Padraic Slattery": [
        "Padraic Slattery <pgoslatara@gmail.com>",
    ],
    "parazyd": [
        "parazyd <parazyd@dyne.org>",
    ],
    "pasta": [
        "pasta <pasta@dashboost.org>",
        "Pasta <pasta@dashboost.org>",
    ],
    "Patrick Brown": [
        "Patrick Brown <patrick.arthur.brown@gmail.com>",
    ],
    "Patrick Kamin": [
        "Patrick Kamin <patrick@caminus.de>",
    ],
    "Patrick Schneider": [
        "p2k",
        "p2k <patrick.p2k.schneider@gmail.com>",
        "Patrick Schneider <patrick.p2k.schneider@gmail.com>",
    ],
    "Patrick Strateman": [
        "Patick Strateman <patrick.strateman@gmail.com>",
        "patrick s <patrick.strateman@gmail.com>",
        "Patrick Strateman <patrick.strateman@gmail.com>",
        "phantomcircuit",
        "phantomcircuit <patrick@cloudhashing.com>",
        "phantomcircuit <phantomcircuit@debian>",
        "pstratem <patrick.strateman@gmail.com>",
    ],
    "Patrick Varilly": [
        "Patrick Varilly <patvarilly@gmail.com>",
    ],
    "Paul Berg": [
        "Paul Berg <paul.berg@inl.gov>",
    ],
    "Paul Georgiou": [
        "Paul Georgiou <pavlos1998@gmail.com>",
    ],
    "Paul Rabahy": [
        "Paul Rabahy",
        "Paul Rabahy <prabahy@gmail.com>",
        "Paul Rabahy <PRabahy@gmail.com>",
    ],
    "Pavel Janík": [
        "Pavel Janík <Pavel@Janik.cz>",
        "paveljanik <Pavel@Janik.cz>",
    ],
    "Pavel Safronov": [
        "Pavel Safronov <pv.safronov@gmail.com>",
    ],
    "Pavel Vasin": [
        "Pavel Vasin",
        "Pavel Vasin <pavel@vasin.nl>",
        "Pavel Vasin <rat4vier@gmail.com>",
    ],
    "Pavlos Antoniou": [
        "Pavlos Antoniou <antoniou-p@hotmail.com>",
    ],
    "Pavol Rusnak": [
        "Pavol Rusnak",
        "Pavol Rusnak <pavol@rusnak.io>",
        "Pavol Rusnak <stick@gk2.sk>",
    ],
    "Pedro Branco": [
        "Pedro Branco",
        "Pedro Branco <branco@uphold.com>",
        "Pedro Branco <pedrobrancolcc@gmail.com>",
    ],
    "Perlover": [
        "Perlover <perlover@perlover.com>",
    ],
    "peryaudo": [
        "peryaudo <peraudo@gmail.com>",
    ],
    "Peter Bushnell": [
        "Bushstar <bushsolo@gmail.com>",
        "Peter Bushnell <bushsolo@gmail.com>",
    ],
    "Peter Josling": [
        "Peter Josling <peterjosling@gmail.com>",
    ],
    "Peter Todd": [
        "Peter Todd <pete@petertodd.org>",
    ],
    "Peter Tschipper": [
        "ptschip",
        "ptschip <peter.tschipper@gmail.com>",
        "Peter Tschipper <peter.tschipper@gmail.com>",
    ],
    "Peter Wagner": [
        "Peter Wagner <puchu@gmx.at>",
    ],
    "Peter Yordanov": [
        "Peter Yordanov <ppyordanov@yahoo.com>",
    ],
    "Peter Zafonte": [
        "Peter Zafonte <pzafonte@pm.me>",
    ],
    "Petter Reinholdtsen": [
        "Petter Reinholdtsen <pere@hungry.com>",
    ],
    "phelixbtc": [
        "phelixbtc <github@blockchained.com>",
    ],
    "Philip Kaufmann": [
        "Philip Kaufmann <phil.kaufmann@t-online.de>",
    ],
    "philsong": [
        "philsong <songbohr@163.com>",
    ],
    "phyBrackets": [
        "phyBrackets <singh.shivamsingh2003@gmail.com>",
    ],
    "Pierre K": [
        "Pierre K <pierrekn@gmail.com>",
        "PiRK <pierrekn@gmail.com>",
    ],
    "Pierre Pronchery": [
        "Pierre Pronchery <khorben@defora.org>",
    ],
    "Pierre Rochard": [
        "Pierre Rochard",
        "Pierre Rochard <pierre.rochard@axial.net>",
        "Pierre Rochard <pierre@rochard.org>",
    ],
    "pierrenn": [
        "pierrenn <git@pnn.sh>",
    ],
    "Pieter Wuille": [
        "Pieter Wuille",
        "sipa",
        "Pieter Wuille <bitcoin-dev@wuille.net>",
        "Pieter Wuille <pieter.wuille@gmail.com>",
        "Pieter Wuille <pieter@wuille.net>",
        "Pieter Wuille <pieterw@google.com>",
        "Pieter Wuille <sipa@ulyssis.org>",
        "sipa <pieter@wuille.net>",
    ],
    "poiuty": [
        "poiuty <poiuty@lepus.su>",
    ],
    "Pol Espinasa": [
        "Pol Espinasa <pol.espinasa@uab.cat>",
    ],
    "poole_party": [
        "poole_party <james@esixteen.co>",
    ],
    "PopeLaz": [
        "PopeLaz <btclz@fastmail.com>",
    ],
    "pox": [
        "pox <pox@xi27pox.org>",
    ],
    "Prabhat Verma": [
        "Prabhat Verma <prabhatverma329@gmail.com>",
    ],
    "pradumnasaraf": [
        "pradumnasaraf <pradumnasaraf@gmail.com>",
    ],
    "Prakash Choudhary": [
        "Prakash Choudhary <44579179+prakash1512@users.noreply.github.com>",
    ],
    "pranabp-bit": [
        "pranabp-bit <pranabp@iitk.ac.in>",
    ],
    "Prateek Sancheti": [
        "Prateek Sancheti <psancheti110@gmail.com>",
    ],
    "Prayag Verma": [
        "Prayag Verma <prayag.verma@gmail.com>",
    ],
    "Prayank": [
        "Prayank",
        "prayank23@outlook.com",
        "prayank@tutanota.de",
        "Prayank <prayank23@outlook.com>",
        "unknown <prayank23@outlook.com>",
        "Prayank <prayank@tutanota.de>",
    ],
    "priscoan": [
        "priscoan <39646804+priscoan@users.noreply.github.com>",
    ],
    "pryds": [
        "pryds <thomas@pryds.eu>",
    ],
    "Pttn": [
        "Pttn <28868425+Pttn@users.noreply.github.com>",
    ],
    "Purple Ninja": [
        "Purple Ninja <129023353+ToRyVand@users.noreply.github.com>",
    ],
    "Puru": [
        "Puru <tuladharpuru@gmail.com>",
    ],
    "pythcoiner": [
        "pythcoiner <pythcoiner@proton.me>",
    ],
    "Qasim Javed": [
        "Qasim Javed <qasimj@gmail.com>",
    ],
    "qmma": [
        "qmma <qmma70@gmail.com>",
    ],
    "qubenix": [
        "qubenix <qubenix@users.noreply.github.com>",
    ],
    "R E Broadley": [
        "R E Broadley <rebroad+github@gmail.com>",
    ],
    "r8921039": [
        "r8921039 <r8921039@hotmail.com>",
    ],
    "Rafael Sadowski": [
        "Rafael Sadowski <rafael@sizeofvoid.org>",
    ],
    "Ragnar": [
        "Ragnar <rodiondenmark@gmail.com>",
    ],
    "Raimo33": [
        "Raimo33 <claudio.raimondi@protonmail.com>",
    ],
    "Randall Naar": [
        "Randall Naar <rnd.naar@gmail.com>",
    ],
    "Randolf Richardson": [
        "Randolf Richardson <randolf@richardson.tw>",
    ],
    "randy-waterhouse": [
        "randy-waterhouse",
        "kiwigb <kiwigb@localhost.localdomain>",
        "randy-waterhouse <kiwigb@yahoo.com>",
        "randy-waterhouse <noone@yodasan>",
    ],
    "RandyMcMillan": [
        "@RandyMcMillan <randy.lee.mcmillan@gmail.com>",
        "randymcmillan <randy.lee.mcmillan@gmail.com>",
        "RandyMcMillan <randy.lee.mcmillan@gmail.com>",
        "randymcmillann <randy.lee.mcmillan@gmail.com>",
    ],
    "Raúl Martínez (RME)": [
        "Raúl Martínez (RME)",
        "Raúl Martínez (RME) <i-rme@users.noreply.github.com>",
        "Raúl Martínez (RME) <rme@rme.li>",
    ],
    "Raul Siles": [
        "Raul Siles <5730357+raulsiles@users.noreply.github.com>",
    ],
    "Rauli Kumpulainen": [
        "Rauli Kumpulainen <rauliweb@gmail.com>",
    ],
    "Rav3nPL": [
        "Rav3nPL <rav3n.pl@gmail.com>",
    ],
    "redshark1802": [
        "redshark1802 <redshark@gmx.org>",
    ],
    "Reese Russell": [
        "Reese Russell <reese.russell@ymail.com>",
        "russeree <reese.russell@ymail.com>",
    ],
    "regergregregerrge": [
        "regergregregerrge <regergregregerrge@oxymail.de>",
    ],
    "Renato Britto": [
        "Renato Britto <renatobritto@protonmail.com>",
    ],
    "René Nyffenegger": [
        "René Nyffenegger",
        "René Nyffenegger <mail@renenyffenegger.ch>",
        "René Nyffenegger <rene.nyffenegger@adp-gmbh.ch>",
    ],
    "Rene Pickhardt": [
        "Rene Pickhardt <r.pickhardt@gmail.com>",
    ],
    "Rhythm Garg": [
        "Rhythm Garg <rhythmgarg05@gmail.com>",
    ],
    "Riahiamirreza": [
        "Riahiamirreza <54557628+Riahiamirreza@users.noreply.github.com>",
    ],
    "Ricardo M. Correia": [
        "Ricardo M. Correia <rcorreia@wizy.org>",
    ],
    "Ricardo Velhote": [
        "Ricardo Velhote <rvelhote@gmail.com>",
    ],
    "Riccardo Masutti": [
        "Riccardo Masutti",
        "Riccardo Masutti <46527252+RiccardoMasutti@users.noreply.github.com>",
        "Riccardo Masutti <riccardo@android-2b910106bc1757ea>",
    ],
    "Riccardo Spagni": [
        "Riccardo Spagni <ric@spagni.net>",
    ],
    "RiceChuan": [
        "RiceChuan <lc582041246@gmail.com>",
    ],
    "Richard Kiss": [
        "Richard Kiss <him@richardkiss.com>",
    ],
    "Richard Myers": [
        "Richard Myers <remyers@yakshaver.org>",
    ],
    "Richard Schwab": [
        "Richard Schwab",
        "Richard Schwab <mail@richardschwab.de>",
        "Richard Schwab <mail@w.tf-w.tf>",
    ],
    "richierichrawr": [
        "richierichrawr <richierichrawr@users.noreply.github.com>",
    ],
    "rion": [
        "rion <rion@cs.stanford.edu>",
    ],
    "riordant": [
        "riordant <riordant@tcd.ie>",
    ],
    "ritickgoenka": [
        "ritickgoenka <rgoenka@ec.iitr.ac.in>",
    ],
    "RJ Rybarczyk": [
        "RJ Rybarczyk <rj@rybar.tech>",
    ],
    "Rjected": [
        "Rjected <kidscline01@gmail.com>",
    ],
    "rkrux": [
        "rkrux <rkrux.connect@gmail.com>",
    ],
    "Rob Fielding": [
        "Rob Fielding <rob@g17.co.nz>",
    ],
    "Rob Van Mieghem": [
        "Rob Van Mieghem <rob@vanmieghemcloud.com>",
    ],
    "Robert": [
        "Robert <robert@lyberry.com>",
    ],
    "Robert Backhaus": [
        "Robert Backhaus <robbak@robbak.com>",
    ],
    "Robert McLaughlin": [
        "Robert McLaughlin <robert@sparkk.us>",
    ],
    "Robert Spigler": [
        "Robert Spigler <RSpigler@ProtonMail.ch>",
    ],
    "Robin David": [
        "Robin David <dev.robin.david@gmail.com>",
    ],
    "RoboSchmied": [
        "RoboSchmied <github@roboschmie.de>",
    ],
    "Rod Vagg": [
        "Rod Vagg <rod@vagg.org>",
    ],
    "rodasmith": [
        "rodasmith <rodasmith@users.noreply.github.com>",
    ],
    "Rojar Smith": [
        "Rojar Smith <rojarsmith@gmail.com>",
    ],
    "Roman Mindalev": [
        "Roman Mindalev <r000n@r000n.net>",
    ],
    "Roman Zeyde": [
        "Roman Zeyde <me@romanzey.de>",
    ],
    "romanornr": [
        "romanornr <romanornr@gmail.com>",
    ],
    "root": [
        "root <root@DESKTOP-48LR9I1.localdomain>",
    ],
    "root (none)": [
        "root <root@Gentoo.(none)>",
    ],
    "Rose Toomey": [
        "Rose Toomey <rktoomey@gmail.com>",
    ],
    "Roy Badami": [
        "Roy Badami <roy@gnomon.org.uk>",
    ],
    "Roy Shao": [
        "Roy Shao <ycshao0402@gmail.com>",
    ],
    "Ruben Dario Ponticelli": [
        "Ruben Dario Ponticelli",
        "Ruben Dario Ponticeli <rdponticelli@gmail.com>",
        "Ruben Dario Ponticelli <rdponticelli@gmail.com>",
        "Rubén Darío Ponticelli <rdponticelli@gmail.com>",
    ],
    "Ruben de Vries": [
        "Ruben de Vries <ruben@rubensayshi.com>",
    ],
    "Rune K Svendsen": [
        "Rune K Svendsen <runesvend@gmail.com>",
        "Rune K. Svendsen <runesvend@gmail.com>",
    ],
    "Russ Yanofsky": [
        "Russ Yanofsky",
        "Russell Yanofsky",
        "Ryan Ofsky",
        "ryanofsky",
        "ryan of sky",
        "russ@yanofsky.org",
        "russ@chaincode.com",
        "ryan@ofsky.org",
        "<Ryan Ofsky <ryan@ofsky.org>",
        "<Ryan Ofsky <ryan@ofsky.org",
        "Ryan Ofsky <ryan@ofsky.org>",
        "ryanofsky <ryan@ofsky.org>",
        "Russ Yanofsky <russ@yanofsky.org>",
        "Russell Yanofsky <russ@chaincode.com>",
        "Russell Yanofsky <russ@yanofsky.org>",
    ],
    "Russell O'Connor": [
        "roconnor-blockstream <roconnor@blockstream.com>",
        "Russell O'Connor <roconnor@blockstream.io>",
    ],
    "rustaceanrob": [
        "rustaceanrob <rob.netzke@gmail.com>",
    ],
    "Rusty Russell": [
        "Rusty Russell <rusty@rustcorp.com.au>",
    ],
    "rxl": [
        "rxl <me@ryanshea.org>",
    ],
    "Ryan Havar": [
        "Ryan Havar",
        "Ryan Havar <rhavar@protonmail.com>",
        "Ryan Havar <ryan@moneypot.com>",
    ],
    "Ryan Loomba": [
        "Ryan Loomba <ryan.loomba@gmail.com>",
    ],
    "Ryan Niebur": [
        "Ryan Niebur <ryanryan52@gmail.com>",
    ],
    "Ryan X. Charles": [
        "Ryan X. Charles <ryanxcharles@gmail.com>",
    ],
    "Saahil Shangle": [
        "Saahil Shangle <saahilshangle@gmail.com>",
    ],
    "sachinkm77": [
        "sachinkm77 <sachinkm77>",
    ],
    "saibato": [
        "saibato <saibato.naga@pm.me>",
        "Saibato <saibato.naga@pm.me>",
    ],
    "Saikiran": [
        "Saikiran <saikirannadipilli@gmail.com>",
    ],
    "Saivann": [
        "Saivann <saivann@gmail.com>",
    ],
    "Salvatore Ingala": [
        "Salvatore Ingala <6681844+bigspider@users.noreply.github.com>",
    ],
    "Samer Afach": [
        "Samer Afach <info@afach.de>",
    ],
    "Samuel B. Atwood": [
        "Samuel B. Atwood <samuel.atwood@gmail.com>",
    ],
    "Samuel Dobson": [
        "MeshCollider <dobsonsa68@gmail.com>",
        "Samuel Dobson <dobsonsa68@gmail.com>",
    ],
    "sandos (none)": [
        "sandos <sandos@sanddesk.(none)>",
    ],
    "Sanjay Ghemawat": [
        "Sanjay Ghemawat <sanjay@google.com>",
    ],
    "Sanjay K": [
        "Sanjay K <sanjaykdragon@gmail.com>",
    ],
    "Sanket Kanjalkar": [
        "sanket1729",
        "Sanket Kanjalkar <sanket1729@gmail.com>",
        "sanket1729 <sanket1729@gmail.com>",
        "sanket1729 <smk7@illinois.edu>",
    ],
    "Santiago M. Mola": [
        "Santiago M. Mola <coldwind@coldwind.org>",
    ],
    "Satoshi Nakamoto": [
        "--author=Satoshi Nakamoto <satoshin@gmx.com>",
        "s_nakamoto",
        "s_nakamoto <s_nakamoto@1a98c847-1fd6-4fd8-948a-caf3550aa51b>",
        "Satoshi Nakamoto <satoshin@gmx.com>",
    ],
    "Sats And Sports": [
        "Sats And Sports <sats.and.sports@gmail.com>",
        "SatsAndSports <satsandsports@gmail.com>",
    ],
    "Sawyer Billings": [
        "Sawyer Billings <sawpaw19@gmail.com>",
    ],
    "scgbckbone": [
        "scgbckbone <scgbckbone@proton.me>",
    ],
    "Scott Ellis": [
        "Scott Ellis <sje397@gmail.com>",
        "sje397 <sje397@gmail.com>",
    ],
    "Scott Howard": [
        "Scott Howard <showard314@gmail.com>",
    ],
    "Scott Willeke": [
        "Scott Willeke <scott@willeke.com>",
    ],
    "sean": [
        "sean <merehap@gmail.com>",
    ],
    "sean-k1": [
        "sean-k1 <uhs2000@naver.com>",
    ],
    "seaona": [
        "seaona <mariona@gmx.es>",
    ],
    "Sebastian Falbesoner": [
        "Sebastian Falbesoner <sebastian.falbesoner@gmail.com>",
        "theStack <sebastian.falbesoner@gmail.com>",
    ],
    "Sebastian Kung": [
        "Sebastian Kung",
        "TheCharlatan",
        "sedited",
        "seb.kung@gmail.com",
        "merge-script <seb.kung@gmail.com>",
    ],
    "secp512k2": [
        "secp512k2 <187356958+secp512k2@users.noreply.github.com>",
    ],
    "seduless": [
        "seduless <seduless@proton.me>",
    ],
    "Sehyun Chung": [
        "Sehyun Chung <sehyun@berkeley.edu>",
    ],
    "Seibart Nedor": [
        "Seibart Nedor <rodentrabies@protonmail.com>",
    ],
    "Seleme Topuz": [
        "Seleme Topuz <seleme94@hotmail.com>",
    ],
    "Sergey Kazenyuk": [
        "Sergey Kazenyuk <kazenyuk@gmail.com>",
    ],
    "Sergi Delgado Segura": [
        "Sergi Delgado Segura <sergi.delgado.s@gmail.com>",
    ],
    "SergioDemianLerner": [
        "SergioDemianLerner <Sergio.d.Lerner@gmail.com>",
    ],
    "setpill": [
        "setpill <37372069+setpill@users.noreply.github.com>",
    ],
    "Sev": [
        "Sev <git@sevastos.com>",
    ],
    "sgulls": [
        "sgulls <abaoagjoojin@outlook.com>",
    ],
    "sh15h4nk": [
        "sh15h4nk <shiva.shvvs@gmail.com>",
    ],
    "Shane Wegner": [
        "Shane Wegner <shane-github@csy.ca>",
    ],
    "shannon1916": [
        "shannon1916 <shannon@unita.network>",
    ],
    "shaolinfry": [
        "shaolinfry <shaolinfry@protonmail.ch>",
    ],
    "Shashwat Vangani": [
        "Shashwat",
        "Shashwat Vangani",
        "85434418+shaavan@users.noreply.github.com",
        "shaavan.github@gmail.com",
        "svangani239@gmail.com",
        "Shashwat Vangani <85434418+shaavan@users.noreply.github.com>",
        "Shashwat <shaavan.github@gmail.com>",
        "Shashwat <svangani239@gmail.com>",
    ],
    "Shaul Kfir": [
        "Shaul Kfir <shaul.kfir@gmail.com>",
    ],
    "Shawn Wilkinson": [
        "Shawn Wilkinson <me@super3.org>",
        "super3 <me@super3.org>",
    ],
    "Shen (none)": [
        "Shen <shen@shen-90X3A.(none)>",
    ],
    "Shigeya Suzuki": [
        "Shigeya Suzuki <shigeya@wide.ad.jp>",
    ],
    "Shooter": [
        "Shooter <shooterman@users.noreply.github.com>",
    ],
    "Shorya": [
        "Shorya <shoryak@iitk.ac.in>",
    ],
    "shshshsh": [
        "shshshsh <shshshsh@sdsdsdfsd.invalid>",
    ],
    "ShubhamPalriwala": [
        "ShubhamPalriwala <spalriwalau@gmail.com>",
    ],
    "Shubhankar Gambhir": [
        "Shubhankar <Shubhankargambhir013@gmail.com>",
        "Shubhankar Gambhir <Shubhankargambhir013@gmail.com>",
    ],
    "Shunsuke Shimizu": [
        "Shunsuke Shimizu <grafi@grafi.jp>",
    ],
    "Simon": [
        "Simon <simon.yao@bitlayer.ltd>",
    ],
    "Simon de la Rouviere": [
        "Simon de la Rouviere <simon@delarouviere.com>",
    ],
    "Simon Males": [
        "Simon Males <sime@sime.net.au>",
    ],
    "Simone Madeo": [
        "Simone Madeo <simone.madeo@gmail.com>",
    ],
    "Sined": [
        "Sined <nightsbird@gmail.com>",
    ],
    "sinetek": [
        "sinetek",
        "pitwuu@gmail.com <root@lstar.my.domain>",
        "sinetek <pitwuu@gmail.com>",
        "sinetek pitwuu@gmail.com",
    ],
    "Sishir Giri": [
        "Sishir Giri <sishirg27@gmail.com>",
        "stackman27 <sishirg27@gmail.com>",
    ],
    "sje": [
        "sje <sje3000@gmail.com>",
    ],
    "Sjors Provoost": [
        "Sjors Provoost",
        "sjors",
        "Sjors Provoost <sjors@sprovoost.nl>",
    ],
    "skmcontrib": [
        "skmcontrib <skmcontrib>",
    ],
    "Skuli Dulfari": [
        "Skuli Dulfari <sdulfari@protonmail.com>",
    ],
    "Smlep": [
        "Smlep <smlep.pro@gmail.com>",
    ],
    "sogoagain": [
        "sogoagain <imyong0@gmail.com>",
    ],
    "SomberNight": [
        "SomberNight <somber.night@protonmail.com>",
    ],
    "soroosh-sdi": [
        "soroosh-sdi <soroosh.sardari@gmail.com>",
    ],
    "Spencer Lievens": [
        "Spencer Lievens <spencerlievens@users.noreply.github.com>",
    ],
    "Sriram": [
        "Sriram <sriramdvt@gmail.com>",
    ],
    "Stacie Waleyko": [
        "Stacie <staciewaleyko@gmail.com>",
        "Stacie Waleyko <1823216+satsie@users.noreply.github.com>",
    ],
    "Stanislas Marion": [
        "Stanislas Marion <stanislas.marion@gmail.com>",
    ],
    "Stefan Richter": [
        "Stefan Richter <stefan@sblbs.de>",
        "stefanwouldgo <stefan@sblbs.de>",
    ],
    "Stepan Snigirev": [
        "Stepan Snigirev <snigirev.stepan@gmail.com>",
    ],
    "Stephan Oeste": [
        "Stephan Oeste <emzy@emzy.de>",
    ],
    "Stéphane Gimenez": [
        "Stéphane Gimenez <dev@gim.name>",
    ],
    "Stephane Glondu": [
        "Stephane Glondu <steph@glondu.net>",
    ],
    "Stephen": [
        "Stephen <scmorse@colby.edu>",
    ],
    "Steve Lee": [
        "Steve Lee <stevenjlee+git@gmail.com>",
    ],
    "Steven": [
        "Steven <steven@sigwo.com>",
    ],
    "Steven D. Lander": [
        "Steven D. Lander <stevendlander@gmail.com>",
    ],
    "Steven Roose": [
        "Steven Roose <steven@stevenroose.org>",
    ],
    "StevenMia": [
        "StevenMia <flite@foxmail.com>",
    ],
    "steverusso": [
        "steverusso <steverusso@protonmail.com>",
    ],
    "stickies-v": [
        "stickies-v",
        "Stickies",
        "stickies-v <69010457+stickies-v@users.noreply.github.com>",
        "stickies-v <stickies-v@protonmail.com>",
        "stickies-v <stickies-v@users.noreply.github.com>",
    ],
    "stratospher": [
        "stratospher",
        "ruhiasap@gmail.com",
        "44024636+stratospher@users.noreply.github.com",
        "= <ruhiasap@gmail.com>",
        "stratospher <ruhiasap@gmail.com>",
        "stratospher <44024636+stratospher@users.noreply.github.com>",
    ],
    "stringintech": [
        "stringintech <stringintech@gmail.com>",
        "Stringintech <stringintech@gmail.com>",
    ],
    "strmfos": [
        "strmfos <155266597+strmfos@users.noreply.github.com>",
    ],
    "Stuart Cardall": [
        "Stuart Cardall <developer@it-offshore.co.uk>",
    ],
    "stutxo": [
        "stutxo <70952638+stutxo@users.noreply.github.com>",
    ],
    "Subo1978": [
        "Subo1978 <shuebbel@gmx.de>",
    ],
    "Suhail Saqan": [
        "Suhail Saqan",
        "Suhail Saqan <suhail.saqan@gmail.com>",
        "Suhail Saqan <suhailsaqan@users.noreply.github.com>",
    ],
    "Suhas Daftuar": [
        "Suhas Daftuar",
        "sdaftuar",
        "sdaftuar@chaincode.com",
        "sdaftuar@gmail.com",
        "Suhas Daftuar <sdaftuar@chaincode.com>",
        "Suhas Daftuar <sdaftuar@gmail.com>",
    ],
    "sunerok": [
        "sunerok <justinvforvendetta@gmail.com>",
    ],
    "Supachai Kheawjuy": [
        "spicyzboss <supachai@spicyz.io>",
        "Supachai Kheawjuy <supachai@spicyz.io>",
    ],
    "Suriyaa Sundararuban": [
        "Suriyaa Kudo",
        "Suriyaa Rocky Sundararuban",
        "Suriyaa Sundararuban",
        "Suriyaa Kudo <SuriyaaKudoIsc@users.noreply.github.com>",
        "Suriyaa Rocky Sundararuban <github@suriyaa.tk>",
        "Suriyaa Sundararuban <github@suriyaa.tk>",
    ],
    "Sven Slootweg": [
        "Sven Slootweg <info@sven-slootweg.nl>",
    ],
    "svost": [
        "svost <ya.nowa@yandex.ru>",
    ],
    "Sylvain Goumy": [
        "Sylvain Goumy <sylvain@uplab.fr>",
    ],
    "t-bast": [
        "t-bast <bastuc@hotmail.fr>",
    ],
    "Taeik Lim": [
        "Taeik Lim <sibera21@gmail.com>",
    ],
    "tailsjoin": [
        "tailsjoin <tailsjoin@users.noreply.github.com>",
    ],
    "Takashi Mitsuta": [
        "Takashi Mitsuta <knhn1117@gmail.com>",
    ],
    "takeshikurosawaa": [
        "takeshikurosawaa <204226757+takeshikurosawaa@users.noreply.github.com>",
    ],
    "Tamas Blummer": [
        "Tamas Blummer",
        "bitsofproof <tamas@bitsofproof.com>",
        "Tamas Blummer <tamas@bitsofproof.com>",
        "Tamas Blummer <tamas.blummer@gmail.com>",
    ],
    "Tariq Bashir": [
        "Tariq Bashir <tariqbashir@gmail.com>",
    ],
    "Tawanda Kembo": [
        "Tawanda Kembo <tawanda@zimstay.com>",
    ],
    "tboy1337": [
        "tboy1337 <tboy1337@proton.me>",
    ],
    "tcatm": [
        "tcatm <tcatm@gawab.com>",
    ],
    "tdb3": [
        "tdb3 <106488469+tdb3@users.noreply.github.com>",
    ],
    "tecnovert": [
        "tecnovert <tecnovert@particl.io>",
    ],
    "Telepatheic": [
        "Telepatheic <thomas@instantsolve.net>",
    ],
    "Teran McKinney": [
        "Teran McKinney <sega01@go-beyond.org>",
    ],
    "TheLazieR Yip": [
        "TheLazieR Yip <thelazier@gmail.com>",
    ],
    "THETCR": [
        "THETCR <thetcr@live.nl>",
    ],
    "Thomas": [
        "Thomas <thomas.giudici@proton.me>",
    ],
    "Thomas Holenstein": [
        "Thomas Holenstein <thomas.holenstein@gmail.com>",
    ],
    "Thomas J": [
        "Thomas J",
        "practicalswift",
        "practicalswift <practicalswift@users.noreply.github.com>",
        "Thomas J <practicalswift@users.noreply.github.com>",
    ],
    "Thomas Kerin": [
        "Thomas Kerin",
        "Thomas Kerin <afk11@users.noreply.github.com>",
        "Thomas Kerin <thomas.kerin@bitmaintech.com>",
    ],
    "Thomas Snider": [
        "Thomas Snider <tjps636@gmail.com>",
    ],
    "Thomas Zander": [
        "Thomas Zander <thomas@thomaszander.se>",
    ],
    "Thoragh": [
        "Thoragh <larssonvictor93@gmail.com>",
    ],
    "tianzedavid": [
        "tianzedavid <cuitianze@aliyun.com>",
    ],
    "Tim Akinbo": [
        "Tim Akinbo <41004+takinbo@users.noreply.github.com>",
    ],
    "Tim Neubauer": [
        "Tim Neubauer <timnbr99@gmail.com>",
    ],
    "Tim Ruffing": [
        "Tim Ruffing",
        "Tim Ruffing <crypto@timruffing.de>",
        "Tim Ruffing <me@real-or-random.org>",
    ],
    "Tim Shimmin": [
        "Tim Shimmin <TimothyShimmin@users.noreply.github.com>",
    ],
    "Timon Rapp": [
        "Timon Rapp <timon@zaeda.net>",
    ],
    "Timothy Redaelli": [
        "Timothy Redaelli",
        "Timothy Redaelli <timothy.redaelli@gmail.com>",
        "Timothy Redaelli <tredaelli@redhat.com>",
    ],
    "Timothy Stranex": [
        "Timothy Stranex <timothy@Timothys-MacBook-Pro.local>",
    ],
    "tm314159": [
        "tm314159 <tm314159@users.noreply.github.com>",
    ],
    "tnaka": [
        "tnaka <nakagat@gmail.com>",
    ],
    "Tobias Kaderle": [
        "Tobias Kaderle <tobias.kaderle@dynatrace.com>",
    ],
    "Tobin Harding": [
        "Tobin Harding <me@tobin.cc>",
    ],
    "tobtoht": [
        "tobtoht <tob@featherwallet.org>",
    ],
    "Tom Harding": [
        "dgenr8",
        "Tom Harding <tomh@thinlink.com>",
    ],
    "Tomás Andróil": [
        "Tomás Andróil <tomasandroil@gmail.com>",
    ],
    "Tomas van der Wansem": [
        "Tomas van der Wansem <tomas@tomasvdw.nl>",
    ],
    "Torhte Butler": [
        "Torhte Butler <torhte@protonmail.com>",
    ],
    "Torkel Rogstad": [
        "Torkel Rogstad <torkel@rogstad.io>",
    ],
    "Torstein Husebø": [
        "Torstein Husebø <torstein@huseboe.net>",
    ],
    "Travin Keith": [
        "Travin Keith <travin@travinkeith.com>",
    ],
    "TrentZ": [
        "TrentZ <wazzytrent@gmail.com>",
    ],
    "Trevin Hofmann": [
        "Trevin Hofmann <trevinhofmann@gmail.com>",
    ],
    "Troy Giorshev": [
        "Troy Giorshev <troygiorshev@gmail.com>",
    ],
    "tryphe": [
        "tryphe",
        "tryphe <tryphe@noreply.github.com>",
        "tryphe <tryphe@users.noreply.github.com>",
    ],
    "tucenaber": [
        "tucenaber <tucenaber@gmail.com>",
    ],
    "tulip": [
        "tulip <tulip@JBinUp.local>",
    ],
    "Tushar Singla": [
        "Tushar Singla <singlatushar07@gmail.com>",
    ],
    "Tyler Chambers": [
        "Tyler Chambers",
        "Tyler Chambers <me@tylerchambers.net>",
        "Tyler Chambers <tyler@iheartapis.com>",
        "Tyler Chambers <tyler@iHeartAPIs.com>",
    ],
    "Tyler Hardin": [
        "Tyler Hardin <th020394@gmail.com>",
    ],
    "U-Zyn Chua": [
        "U-Zyn Chua <chua@uzyn.com>",
    ],
    "UdjinM6": [
        "UdjinM6",
        "UdjinM6 <UdjinM6@dash.org>",
        "UdjinM6 <UdjinM6@users.noreply.github.com>",
    ],
    "Ulrich Kempken": [
        "Ulrich Kempken <Uli.Kempken@t-online.de>",
    ],
    "umiumi": [
        "umiumi <9@umi.cat>",
    ],
    "unknown (none)": [
        "unknown <Administrator@.(none)>",
    ],
    "unsystemizer": [
        "unsystemizer",
        "unsystemizer <something@gmail.com>",
        "unsystemizer <unsystemizer@users.noreply.github.com>",
    ],
    "Uplab": [
        "Uplab <uplab@macbook-pro-de-uplab.home>",
    ],
    "Utsav Gupta": [
        "Utsav Gupta <utsavgupta89@gmail.com>",
    ],
    "Vaclav Vobornik": [
        "Vaclav Vobornik <git@vobornik.eu>",
    ],
    "Vadim Peretokin": [
        "Vadim Peretokin <vperetokin@gmail.com>",
    ],
    "Varunram Ganesh": [
        "Varunram <vrg2009@ymail.com>",
        "Varunram Ganesh <vrg2009@ymail.com>",
    ],
    "Vasil Dimov": [
        "Vasil Dimov",
        "vasild",
        "vd@freebsd.org",
        "Vasil Dimov <vd@FreeBSD.org>",
        "Vasil Dimov <vd@freebsd.org>",
        "vasild <vd@FreeBSD.org>",
    ],
    "Vasil Stoyanov": [
        "Vasil Stoyanov <vasil.stoyanov@protonmail.com>",
    ],
    "Vegard Nossum": [
        "Vegard Nossum <vegard.nossum@gmail.com>",
    ],
    "Venkatesh Srinivas": [
        "Venkatesh Srinivas <me@endeavour.zapto.org>",
    ],
    "Veres Lajos": [
        "Veres Lajos <vlajos@gmail.com>",
    ],
    "Victor Felder": [
        "vhf / victor felder <victorfelder@gmail.com>",
        "Victor Felder <victorfelder@gmail.com>",
    ],
    "Victor Leschuk": [
        "Victor Leschuk <vleschuk@gmail.com>",
    ],
    "Vidar Holen": [
        "Vidar Holen <spam@vidarholen.net>",
    ],
    "vim88": [
        "vim88 <vim88vim88@gmail.com>",
    ],
    "ViniciusCestarii": [
        "ViniciusCestarii <viniciuscestari01@gmail.com>",
    ],
    "Vinnie Falco": [
        "Vinnie Falco <vinnie.falco@gmail.com>",
    ],
    "Virgil Dupras": [
        "Virgil Dupras <hsoft@hardcoded.net>",
    ],
    "virtu": [
        "virtu <virtu@cryptic.to>",
    ],
    "Vitalii Demianets": [
        "Vitalii Demianets <vitalii@orsoc.se>",
    ],
    "Vivek Ganesan": [
        "Vivek Ganesan <caliberoviv@gmail.com>",
        "vivganes <vivek@vivekganesan.com>",
    ],
    "VolodymyrBg": [
        "VolodymyrBg <aqdrgg19@gmail.com>",
    ],
    "vuittont60": [
        "vuittont60 <vuittontvuittont50@outlook.com>",
    ],
    "w0xlt": [
        "@w0xlt <w0xlt@users.noreply.github.com>",
        "w0xlt",
        "w0xlt@users.noreply.github.com",
        "94266259+w0xlt@users.noreply.github.com",
        "woltx",
        "woltx@protonmail.com",
        "w0xlt <w0xlt@users.noreply.github.com>",
        "w0xlt <94266259+w0xlt@users.noreply.github.com>",
        "woltx <94266259+w0xlt@users.noreply.github.com>",
        "w0xlt <woltx@protonmail.com>",
    ],
    "WakeTrainDev": [
        "WakeTrainDev <175499942+waketraindev@users.noreply.github.com>",
    ],
    "Walter": [
        "Walter <s.heinhuis@outlook.com>",
    ],
    "Warren Togami": [
        "Warren Togami <wtogami@gmail.com>",
    ],
    "Weixie Cui": [
        "Weixie Cui <cuiweixie@gmail.com>",
    ],
    "Werner Lemberg": [
        "Werner Lemberg <wl@gnu.org>",
    ],
    "wgyt": [
        "wgyt <wgythe@gmail.com>",
    ],
    "Whit J": [
        "Whit J <whitj00@users.noreply.github.com>",
    ],
    "whiteh0rse": [
        "whiteh0rse <95139855+whiteh0rse@users.noreply.github.com>",
    ],
    "Wil Bown": [
        "Wil Bown <wilbown@users.noreply.github.com>",
    ],
    "Will Ayd": [
        "Will Ayd <william.ayd@icloud.com>",
    ],
    "Will Binns": [
        "Will Binns",
        "Will Binns <binns@21.co>",
        "Will Binns <will@trek.io>",
    ],
    "Will Clark": [
        "Will Clark",
        "will@256k1.dev",
        "will8clark@gmail.com",
        "will <will@256k1.dev>",
        "willcl-ark",
        "Will Clark <will8clark@gmail.com>",
        "willcl-ark <will8clark@gmail.com>",
        "willcl-ark <will@256k1.dev>",
    ],
    "William Bright": [
        "William Bright <wbright@protonmail.com>",
    ],
    "William Casarin": [
        "William Casarin <jb55@jb55.com>",
    ],
    "William Robinson": [
        "William Robinson <wbarobinson@gmail.com>",
    ],
    "William Yager": [
        "William Yager <will.yager@gmail.com>",
    ],
    "Willy Ko": [
        "Willy Ko",
        "Willy Ko <k.o.willy@gmail.com>",
        "Willy Ko <wko@blockchainfoundry.co>",
        "willyk <wko@blockchainfoundry.co>",
    ],
    "Wilson Ccasihue S": [
        "hel0 <wilson2cs@gmail.com>",
        "Wilson Ccasihue S <wilson2cs@gmail.com>",
    ],
    "winder": [
        "winder <wwinder.unh@gmail.com>",
    ],
    "windsok": [
        "windsok <windsok@protonmail.com>",
    ],
    "Witchspace": [
        "Witchspace <witchspace81@gmail.com>",
    ],
    "wiz": [
        "wiz <j@wiz.biz>",
    ],
    "wodry": [
        "wodry",
        "wodry <wodry@localhost>",
        "wodry <wodry@users.noreply.github.com>",
    ],
    "Woolfgm": [
        "Woolfgm <160153877+Dahka2321@users.noreply.github.com>",
    ],
    "xanatos": [
        "xanatos <xanatos@geocities.com>",
    ],
    "Yahia Chiheb": [
        "Yahia Chiheb <chihebyahia@gmail.com>",
    ],
    "Yancy Ribbens": [
        "yancy",
        "yancy <github@yancy.lol>",
        "yancy <yancy@yancy.lol>",
        "Yancy Ribbens <yancy.ribbens@gmail.com>",
    ],
    "Yash Bhutwala": [
        "Yash Bhutwala <yash.bhutwala@gmail.com>",
    ],
    "Yerzhan Mazhkenov": [
        "Yerzhan Mazhkenov <20302932+yerzhan7@users.noreply.github.com>",
    ],
    "Yoichi Hirai": [
        "Yoichi Hirai <i@yoichihirai.com>",
    ],
    "yongxinyao": [
        "yongxinyao <yyxyong@163.com>",
    ],
    "Yuri Zhykin": [
        "whythat",
        "whythat <whythat@protonmail.com>",
        "whythat <yuri.zhykin@gmail.com>",
        "Yuri Zhykin <yuri.zhykin@gmail.com>",
    ],
    "Yusuf Sahin HAMZA": [
        "Yusuf Sahin HAMZA <yusufsahinhamza@gmail.com>",
    ],
    "Yuval Kogman": [
        "Yuval Kogman <nothingmuch@woobling.org>",
    ],
    "zaidmstrr": [
        "zaidmstrr <zaidbrock122@gmail.com>",
    ],
    "Zain Iqbal Allarakhia": [
        "Zain Iqbal Allarakhia",
        "Zain Iqbal Allarakhia <zain.allarakhia@gmail.com>",
        "Zain Iqbal Allarakhia <zain@za1.co>",
    ],
    "Zak Wilcox": [
        "Zak Wilcox <iwilcox@iwilcox.me.uk>",
    ],
    "Zakk": [
        "Zakk <zakklakin@outlook.com>",
    ],
    "zathras-crypto": [
        "zathras-crypto <zathrasc@gmail.com>",
    ],
    "zealsham": [
        "zealsham <shammahagwor@gmail.com>",
    ],
    "zenosage": [
        "zenosage <zenosage@protonmail.com>",
    ],
    "Zero": [
        "Zero <zero1729@protonmail.com>",
        "Zero-1729 <zero1729@protonmail.com>",
    ],
    "zhaohaitao": [
        "zhaohaitao <zhaohaitao@huobi.com>",
    ],
    "“jkcd”": [
        "“jkcd” <“jkreadsemail@gmail.com”>",
    ],
    "ロハン ダル": [
        "ロハン ダル <rohun-dhar@MN14042102.local>",
    ],
}


COAUTHOR_RE = re.compile(r"^Co-authored-by:\s*(.*(?:\n[ \t]+.*)*)", re.IGNORECASE | re.MULTILINE)
GITHUB_NOREPLY_RE = re.compile(r"(?:(?:\d+)\+)?([^@]+)@users\.noreply\.github\.com$")
EMAIL_RE = re.compile(r"^[^\s@<>]+@[^\s@<>]+$")
QUOTE_PAIRS = {
    '"': '"',
    "'": "'",
    "“": "”",
    "‘": "’",
}


@dataclass(frozen=True)
class Identity:
    name: str
    email: str


@dataclass(frozen=True)
class CommitInfo:
    author: Identity
    parents: list[str]
    body: str


@dataclass
class ContributorStats:
    own_commits: int = 0
    coauthored_commits: int = 0
    merges: int = 0
    identities: Counter[str] = field(default_factory=Counter)

    @property
    def total(self) -> int:
        return self.own_commits + self.coauthored_commits + self.merges

    def rank_count(self, *, include_merges: bool) -> int:
        total = self.own_commits + self.coauthored_commits
        if include_merges:
            total += self.merges
        return total


def normalize(value: str) -> str:
    value = unicodedata.normalize("NFKD", value)
    value = "".join(ch for ch in value if not unicodedata.combining(ch))
    value = value.casefold().strip()
    value = value.removeprefix("@")
    return re.sub(r"\s+", " ", value)


def strip_wrapping_quotes(text: str) -> str:
    text = text.strip()
    while len(text) >= 2 and QUOTE_PAIRS.get(text[0]) == text[-1]:
        text = text[1:-1].strip()
    return text


def parse_name_email(text: str) -> tuple[str, str]:
    text = strip_wrapping_quotes(text)
    name, email = parseaddr(text)
    if not EMAIL_RE.fullmatch(email) and "<" in email and ">" in email:
        name, email = parseaddr(strip_wrapping_quotes(email))
    if EMAIL_RE.fullmatch(email):
        return name.strip(), email.strip()

    if text.startswith("<"):
        name, email = parseaddr(text[1:])
        if EMAIL_RE.fullmatch(email):
            return name.strip(), email.strip()

    parts = text.rsplit(maxsplit=1)
    if len(parts) == 2 and EMAIL_RE.fullmatch(parts[1]):
        return parts[0].strip(), parts[1].strip()

    return text.strip(), ""


def build_alias_map() -> dict[str, str]:
    aliases: dict[str, str] = {}
    for canonical, variants in SAME_AUTHORS.items():
        for variant in variants:
            aliases[normalize(variant)] = canonical
            _name, email = parse_name_email(variant)
            if email:
                aliases[email.casefold()] = canonical
    return aliases


ALIASES = build_alias_map()


def run_git(repo: str, args: list[str]) -> bytes:
    process = subprocess.run(
        ["git", "-C", repo, *args],
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return process.stdout


def parse_identity(text: str) -> Identity:
    name, email = parse_name_email(text)
    return Identity(name=name.strip(), email=email.strip())


def identity_keys(identity: Identity) -> list[str]:
    keys = [normalize(display_identity(identity))]
    if identity.email:
        email = identity.email.casefold()
        keys.append(email)
        if match := GITHUB_NOREPLY_RE.fullmatch(email):
            keys.append(normalize(match.group(1)))
    if identity.name:
        keys.append(normalize(identity.name))
    return keys


def display_identity(identity: Identity) -> str:
    if identity.name and identity.email:
        return f"{identity.name} <{identity.email}>"
    return identity.name or identity.email or "(unknown)"


def canonicalize(identity: Identity) -> tuple[str, str]:
    for key in identity_keys(identity):
        if key in ALIASES:
            canonical = ALIASES[key]
            return canonical, canonical
    display = display_identity(identity)
    if identity.email:
        return f"email:{identity.email.casefold()}", display
    return f"name:{normalize(identity.name)}", display


def identities_from_address_text(text: str) -> list[Identity]:
    addresses = getaddresses([text])
    if len(addresses) > 1 and all(EMAIL_RE.fullmatch(email.strip()) for _name, email in addresses):
        return [Identity(name.strip(), email.strip()) for name, email in addresses]

    matches = list(re.finditer(r"([^<>\n]+?)\s*<([^<>\s]+@[^<>\s]+)>", text))
    if len(matches) > 1:
        return [Identity(match.group(1).strip(), match.group(2).strip()) for match in matches]
    if len(matches) == 1 and matches[0].group(0).strip() == text:
        return [Identity(matches[0].group(1).strip(), matches[0].group(2).strip())]

    return [parse_identity(text)]


def coauthors_from_body(body: str) -> list[Identity]:
    coauthors: list[Identity] = []
    for match in COAUTHOR_RE.finditer(body):
        text = strip_wrapping_quotes(match.group(1))
        if not text:
            continue
        coauthors.extend(identities_from_address_text(text))
    return coauthors


def commit_hashes(repo: str, ref: str, max_count: int | None) -> list[str]:
    args = ["rev-list", ref]
    if max_count is not None:
        args.insert(1, f"--max-count={max_count}")
    return run_git(repo, args).decode().splitlines()


def commit_metadata(repo: str, ref: str, max_count: int | None) -> dict[str, CommitInfo]:
    args = ["log", "--format=%H%x1f%an%x1f%ae%x1f%P%x1f%B%x1e"]
    if max_count is not None:
        args.insert(1, f"--max-count={max_count}")
    args.append(ref)

    output = run_git(repo, args).decode("utf-8", "replace")
    metadata: dict[str, CommitInfo] = {}
    for record in output.split("\x1e"):
        if not record.strip("\n"):
            continue
        commit, author_name, author_email, parents, body = record.split("\x1f", 4)
        metadata[commit.strip()] = CommitInfo(
            author=Identity(author_name.strip(), author_email.strip()),
            parents=parents.split(),
            body=body,
        )
    return metadata


def changed_paths_by_commit(repo: str, commits: list[str]) -> dict[str, list[str]]:
    if not commits:
        return {}

    process = subprocess.run(
        ["git", "-C", repo, "diff-tree", "--stdin", "--root", "--name-only", "-r", "-m", "-z"],
        input=("\n".join(commits) + "\n").encode(),
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    commit_set = set(commits)
    paths_by_commit: dict[str, list[str]] = {commit: [] for commit in commits}
    current_commit: str | None = None
    for token in process.stdout.decode("utf-8", "replace").split("\0"):
        if not token:
            continue
        if token in commit_set:
            current_commit = token
        elif current_commit is not None and not re.fullmatch(r"[0-9a-f]{40}", token):
            paths_by_commit[current_commit].append(token)
    return paths_by_commit


def external_path(path: str) -> bool:
    return any(path == prefix.rstrip("/") or path.startswith(prefix) for prefix in EXTERNAL_PREFIXES)


def external_only(paths: list[str]) -> bool:
    return bool(paths) and all(external_path(path) for path in paths)


def count_contributors(
    args: argparse.Namespace,
) -> tuple[dict[str, ContributorStats], dict[str, str], dict[str, int]]:
    stats: dict[str, ContributorStats] = defaultdict(ContributorStats)
    labels: dict[str, str] = {}
    totals = {
        "scanned": 0,
        "counted": 0,
        "skipped_external": 0,
        "merge_commits": 0,
    }

    commits = commit_hashes(args.repo, args.ref, args.max_count)
    metadata = commit_metadata(args.repo, args.ref, args.max_count)
    paths_by_commit = changed_paths_by_commit(args.repo, commits)

    for commit in commits:
        totals["scanned"] += 1
        info = metadata[commit]
        is_merge = len(info.parents) > 1

        paths = paths_by_commit[commit]
        if not args.include_external and external_only(paths):
            totals["skipped_external"] += 1
            continue

        if is_merge:
            totals["merge_commits"] += 1

        author_key, author_label = canonicalize(info.author)
        labels.setdefault(author_key, author_label)
        stats[author_key].identities[display_identity(info.author)] += 1

        coauthor_keys: set[str] = set()
        for coauthor in coauthors_from_body(info.body):
            coauthor_key, coauthor_label = canonicalize(coauthor)
            labels.setdefault(coauthor_key, coauthor_label)
            stats[coauthor_key].identities[display_identity(coauthor)] += 1
            if coauthor_key != author_key:
                coauthor_keys.add(coauthor_key)

        if is_merge:
            # A merge counts once regardless of how many commits it brings in.
            for key in {author_key, *coauthor_keys}:
                stats[key].merges += 1
        else:
            stats[author_key].own_commits += 1
            for key in coauthor_keys:
                stats[key].coauthored_commits += 1
        totals["counted"] += 1

    for key, label in labels.items():
        stats[key].identities[label] += 0
    return stats, labels, totals


def sorted_rows(
    stats: dict[str, ContributorStats],
    labels: dict[str, str],
    *,
    include_merges: bool,
) -> list[tuple[str, ContributorStats]]:
    return sorted(
        ((labels.get(key, key), value) for key, value in stats.items()),
        key=lambda row: (-row[1].rank_count(include_merges=include_merges), row[0].casefold()),
    )


def print_table(
    stats: dict[str, ContributorStats],
    labels: dict[str, str],
    totals: dict[str, int],
    args: argparse.Namespace,
) -> None:
    rows = sorted_rows(stats, labels, include_merges=args.include_merges)
    if args.limit is not None:
        rows = rows[: args.limit]

    print(f"ref: {args.ref}")
    print(f"commits scanned: {totals['scanned']}")
    print(f"commits counted: {totals['counted']}")
    print(f"merge commits counted: {totals['merge_commits']}")
    print(f"external-only commits skipped: {totals['skipped_external']}")
    print()
    print(f"{'rank':>4} {'total':>7} {'own':>7} {'coauth':>7} {'merges':>7}  contributor")
    for rank, (label, value) in enumerate(rows, start=1):
        if value.total < args.min_count:
            continue
        print(
            f"{rank:>4} {value.total:>7} {value.own_commits:>7} "
            f"{value.coauthored_commits:>7} {value.merges:>7}  {label}"
        )
        if args.show_identities:
            identities = ", ".join(
                identity for identity, _count in value.identities.most_common() if identity != label
            )
            if identities:
                print(f"{'':>4} {'':>7} {'':>7} {'':>7} {'':>7}  aka: {identities}")


def print_json(
    stats: dict[str, ContributorStats],
    labels: dict[str, str],
    totals: dict[str, int],
    args: argparse.Namespace,
) -> None:
    rows = []
    for label, value in sorted_rows(stats, labels, include_merges=args.include_merges):
        if value.total < args.min_count:
            continue
        rows.append(
            {
                "contributor": label,
                "total": value.total,
                "own_commits": value.own_commits,
                "coauthored_commits": value.coauthored_commits,
                "merges": value.merges,
                "rank_count": value.rank_count(include_merges=args.include_merges),
                "identities": dict(value.identities.most_common()),
            }
        )
    if args.limit is not None:
        rows = rows[: args.limit]
    print(json.dumps({"ref": args.ref, "totals": totals, "contributors": rows}, indent=2))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", default=".", help="Git repository to inspect. Default: current directory.")
    parser.add_argument(
        "--ref",
        default="HEAD",
        help="Ref to walk backwards from. Default: HEAD.",
    )
    parser.add_argument("--max-count", type=int, help="Limit commits walked, useful for quick checks.")
    parser.add_argument("--limit", type=int, help="Limit rows printed.")
    parser.add_argument("--min-count", type=int, default=1, help="Hide contributors below this count.")
    parser.add_argument(
        "--include-external",
        action="store_true",
        help="Include commits that only touch imported third-party subtrees.",
    )
    parser.add_argument(
        "--include-merges",
        action="store_true",
        help="Include merge counts in ranking. Merge counts are always displayed.",
    )
    parser.add_argument(
        "--show-identities",
        action="store_true",
        help="Show raw names/emails grouped under each canonical contributor.",
    )
    parser.add_argument("--json", action="store_true", help="Emit JSON instead of a table.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        stats, labels, totals = count_contributors(args)
    except subprocess.CalledProcessError as error:
        sys.stderr.write(error.stderr.decode("utf-8", "replace"))
        return error.returncode

    if args.json:
        print_json(stats, labels, totals, args)
    else:
        print_table(stats, labels, totals, args)
    return 0


if __name__ == "__main__":
    sys.exit(main())
