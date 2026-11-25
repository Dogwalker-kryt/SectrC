# Turns the captured terminal output (*.ans) into PNGs and a GIF: render.py <input dir> <output dir>
import re, sys, os
from PIL import Image, ImageDraw, ImageFont
from fontTools.ttLib import TTFont
J="/usr/share/fonts/truetype/jetbrains-mono/JetBrainsMono-"; D="/usr/share/fonts/truetype/dejavu/DejaVuSansMono"
BG=(22,22,30); FG=(205,208,220); BAR=(34,34,46)
PAL={31:(243,99,113),32:(126,211,124),33:(240,192,96),34:(110,160,250),35:(214,130,230),36:(92,207,220),90:(118,122,142)}
X256={99:(150,110,255),103:(135,135,175),245:(138,138,138)}
class R:
    def __init__(s,size):
        s.f={False:ImageFont.truetype(J+"Regular.ttf",size),True:ImageFont.truetype(J+"Bold.ttf",size)}
        s.fb={False:ImageFont.truetype(D+".ttf",size),True:ImageFont.truetype(D+"-Bold.ttf",size)}
        s.cm=TTFont(J+"Regular.ttf").getBestCmap(); s.cw=round(s.f[False].getlength("M")); s.lh=round(size*1.36); s.sz=size
    def draw(s,grid,cols,rows,title,cur=None):
        p=s.sz; top=round(s.sz*2.1); w=cols*s.cw+2*p; h=top+rows*s.lh+p
        im=Image.new("RGB",(w,h),BG); d=ImageDraw.Draw(im); d.rectangle((0,0,w,top-round(s.sz*.5)),fill=BAR)
        r=s.sz*.33; cy=(top-s.sz*.5)/2
        for i,c in enumerate([(255,95,87),(254,188,46),(40,200,64)]):
            cx=p+r+i*s.sz*1.25; d.ellipse((cx-r,cy-r,cx+r,cy+r),fill=c)
        tw=s.f[False].getlength(title); d.text(((w-tw)/2,cy),title,font=s.f[False],fill=(150,153,170),anchor="lm")
        for y,row in enumerate(grid):
            for x,cell in row.items():
                ch,col,b=cell
                if ch==" ": continue
                X=p+x*s.cw; Y=top+y*s.lh
                if ch in "█░":
                    c=col if ch=="█" else tuple((a+3*q)//4 for a,q in zip(col,BG))
                    d.rectangle((X,Y+s.lh*.14,X+s.cw-1,Y+s.lh*.86),fill=c); continue
                if ch in "─━":
                    t=max(1,round(s.sz*(.07 if ch=="─" else .16))); m=Y+s.lh/2
                    d.rectangle((X,m-t/2,X+s.cw-1,m+t/2),fill=col); continue
                f=(s.f if ord(ch) in s.cm else s.fb)[b]; d.text((X,Y+s.lh/2+s.sz*.36),ch,font=f,fill=col,anchor="ls")
        if cur: X=p+cur[0]*s.cw; Y=top+cur[1]*s.lh; d.rectangle((X,Y+s.lh*.12,X+s.cw-2,Y+s.lh*.88),fill=(170,170,190))
        return im
def run(txt):
    grid=[{}];x=y=0;col=FG;b=False;snaps=[]
    for m in re.finditer(r"\x1e(\d+);|\x1b\[([\d;]*)m|(.)",txt,re.S):
        if m[1]: snaps.append((int(m[1]),[dict(r) for r in grid],(x,y)))
        elif m[2] is not None:
            q=[int(v or 0) for v in m[2].split(";")];i=0
            while i<len(q):
                v=q[i]
                if v==0: col=FG;b=False
                elif v==1: b=True
                elif v==38: col=X256.get(q[i+2],FG); i+=2
                elif v in PAL: col=PAL[v]
                i+=1
        else:
            ch=m[3]
            if ch=="\n": y+=1;x=0;grid+= [{}] if y>=len(grid) else []
            elif ch=="\r": x=0
            else: grid[y][x]=(ch,col,b); x+=1
    while grid and not any(c[0]!=" " for c in grid[-1].values()): grid.pop()
    return grid,snaps
def width(g): return max((x for r in g for x,c in r.items() if c[0]!=" "),default=0)+2
def cmdline(c): return f"\x1b[1;32m$\x1b[0m {c}\n"
src,dst=sys.argv[1:3]; os.makedirs(dst,exist_ok=True)
for n,c in [("list","sectrc --list"),("nvme","sudo sectrc --health nvme0n1"),("ata","sudo sectrc --health sda"),("info","sectrc --info nvme0n1")]:
    g,_=run(cmdline(c)+open(f"{src}/{n}.ans").read()); fn={"nvme":"health-ok","ata":"health-failing"}.get(n,n)
    R(28).draw(g,max(width(g),64),len(g),"sectrc").save(f"{dst}/{fn}.png",optimize=True)
g,sn=run(open(f"{src}/clone.ans",newline="").read()); cols=max(width(g),82); rows=len(g); r=R(17)
fr=[r.draw(s,cols,rows,"sectrc",c if c[1]<rows else None) for _,s,c in sn]
pal=fr[-1].quantize(48); mid=fr[len(fr)//2]
both=Image.new("RGB",(fr[0].width,fr[0].height*2)); both.paste(mid,(0,0)); both.paste(fr[-1],(0,fr[0].height)); ImageDraw.Draw(both).rectangle((300,0,420,20),fill=(255,95,87)); pal=both.quantize(96)
q=[f.quantize(palette=pal,dither=0) for f in fr]
q[0].save(f"{dst}/clone.gif",save_all=True,append_images=q[1:],duration=[m for m,*_ in sn],loop=0,optimize=False,disposal=1)
