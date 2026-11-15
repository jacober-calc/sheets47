#!/usr/bin/env python3
"""Interactive host view of the actual application renderer and Spreader backend.

Uses standard Python tkinter; build with tools/build.py host first. All worksheet
data and display output remain under build/host/. Does not emulate DMCP5 hardware.
"""
from pathlib import Path
import subprocess
import tkinter as tk

ROOT=Path(__file__).resolve().parents[1]
FRAME=ROOT/'build/host/viewer-frame.pbm'


def main():
    process=subprocess.Popen([str(ROOT/'build/host/sheets47-tests'),'--repl',str(ROOT/'build/host/SHEETS47')],cwd=ROOT,stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True,bufsize=1)
    if process.stdout.readline().strip()!='READY':raise RuntimeError('Host application did not start')
    window=tk.Tk();window.title('sheets47 0.1.0 — host view (not hardware emulation)')
    screen=tk.Label(window,borderwidth=0);screen.pack()
    def command(value):
        process.stdin.write(value+'\n');process.stdin.flush()
        while True:
            reply=process.stdout.readline()
            if not reply:raise RuntimeError('Host process exited')
            if reply.strip()=='OK':break
    def redraw():
        command('frame '+str(FRAME))
        with FRAME.open('rb') as source:
            assert source.readline()==b'P4\n' and source.readline()==b'400 240\n'
            bits=source.read()
        data=bytearray()
        for byte in bits:
            for shift in range(7,-1,-1):data.extend(b'\0\0\0' if byte&(1<<shift) else b'\xff\xff\xff')
        picture=tk.PhotoImage(data=b'P6\n400 240\n255\n'+data,format='PPM').zoom(2,2)
        screen.configure(image=picture);screen.image=picture
    def key(name):command('key '+name);redraw()
    buttons=tk.Frame(window);buttons.pack(fill='x')
    for number in range(1,7):tk.Button(buttons,text=f'F{number}',command=lambda n=number:key(f'F{n}')).pack(side='left',expand=True,fill='x')
    controls=tk.Frame(window);controls.pack()
    for label,name in [('Left','LEFT'),('Up','UP'),('Down','DOWN'),('Right','RIGHT'),('Enter','ENTER'),('Backspace','BSP'),('Exit','EXIT')]:
        tk.Button(controls,text=label,command=lambda n=name:key(n)).pack(side='left')
    tk.Label(window,text='Arrow keys navigate. Enter edits/commits. F1–F6 softkeys. Escape cancels. FILE → DEMO opens sample.').pack()
    direct=tk.Entry(window,width=65);direct.pack(side='left',fill='x',expand=True)
    tk.Button(window,text='Type into current editor',command=lambda:(command('text '+direct.get()),direct.delete(0,'end'),redraw())).pack(side='right')
    mapping={'Up':'UP','Down':'DOWN','Left':'LEFT','Right':'RIGHT','Return':'ENTER','Escape':'EXIT','BackSpace':'BSP',**{f'F{i}':f'F{i}' for i in range(1,7)}}
    def keyboard(event):
        if window.focus_get()==direct:return
        if event.keysym in mapping:key(mapping[event.keysym]);return 'break'
        if event.char and 32<=ord(event.char)<=126:command('text '+event.char);redraw();return 'break'
    window.bind('<Key>',keyboard)
    def close():
        # Explicit Save remains a user command; closing the viewer deletes no files.
        process.stdin.write('quit\n');process.stdin.flush();process.wait(timeout=5);window.destroy()
    window.protocol('WM_DELETE_WINDOW',close);redraw();window.mainloop()


if __name__=='__main__':main()
