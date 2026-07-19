using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Drawing.Text;
using System.IO;

class Gen {
  static GraphicsPath RoundRect(float x, float y, float w, float h, float r) {
    var gp = new GraphicsPath();
    float d = r * 2f;
    gp.AddArc(x, y, d, d, 180, 90);
    gp.AddArc(x + w - d, y, d, d, 270, 90);
    gp.AddArc(x + w - d, y + h - d, d, d, 0, 90);
    gp.AddArc(x, y + h - d, d, d, 90, 90);
    gp.CloseFigure();
    return gp;
  }

  static void DrawCard(Graphics g, float cx, float cy, float w, float h, float angle, Color fill, Color border, string letter, float fontSize) {
    var state = g.Save();
    g.TranslateTransform(cx, cy);
    g.RotateTransform(angle);
    float x = -w/2f, y = -h/2f, r = Math.Max(8f, w*0.08f);

    using (var shadow = RoundRect(x+6, y+10, w, h, r))
    using (var sb = new SolidBrush(Color.FromArgb(100,0,0,0)))
      g.FillPath(sb, shadow);

    using (var path = RoundRect(x, y, w, h, r)) {
      using (var fb = new SolidBrush(fill)) g.FillPath(fb, path);
      using (var lg = new LinearGradientBrush(new RectangleF(x,y,w,h), Color.FromArgb(90,255,255,255), Color.FromArgb(0,0,0,0), 90f))
        g.FillPath(lg, path);
      using (var pen = new Pen(border, Math.Max(4f, w*0.05f))) {
        pen.LineJoin = LineJoin.Round;
        g.DrawPath(pen, path);
      }
      // inner gold line
      using (var inner = RoundRect(x+w*0.06f, y+h*0.06f, w*0.88f, h*0.88f, r*0.7f))
      using (var pen2 = new Pen(Color.FromArgb(180, border.R, border.G, border.B), Math.Max(1.5f, w*0.015f)))
        g.DrawPath(pen2, inner);

      using (var bb = new SolidBrush(Color.FromArgb(180, border.R, border.G, border.B)))
        g.FillRectangle(bb, x+w*0.12f, y+h*0.1f, w*0.76f, h*0.09f);
    }

    // Letter upright (counter-rotate so K/T/V always readable)
    if (!string.IsNullOrEmpty(letter)) {
      g.RotateTransform(-angle);
      using (var font = new Font("Segoe UI", fontSize, FontStyle.Bold, GraphicsUnit.Pixel))
      using (var sf = new StringFormat { Alignment = StringAlignment.Center, LineAlignment = StringAlignment.Center })
      using (var outline = new SolidBrush(border))
      using (var ink = new SolidBrush(Color.FromArgb(255,255,248,230))) {
        var rect = new RectangleF(-w*0.45f, -h*0.35f, w*0.9f, h*0.85f);
        for (int ox=-3; ox<=3; ox++)
        for (int oy=-3; oy<=3; oy++) {
          if (ox*ox+oy*oy > 10) continue;
          if (ox==0 && oy==0) continue;
          g.DrawString(letter, font, outline, new RectangleF(rect.X+ox, rect.Y+oy, rect.Width, rect.Height), sf);
        }
        g.DrawString(letter, font, ink, rect, sf);
      }
    }

    g.Restore(state);
  }

  static void WriteIcon(int size, string path) {
    using (var bmp = new Bitmap(size, size))
    using (var g = Graphics.FromImage(bmp)) {
      g.SmoothingMode = SmoothingMode.AntiAlias;
      g.TextRenderingHint = TextRenderingHint.AntiAliasGridFit;
      g.Clear(Color.FromArgb(255,10,5,24));

      using (var pathGlow = new GraphicsPath()) {
        pathGlow.AddEllipse(size*0.06f, size*0.06f, size*0.88f, size*0.88f);
        using (var pgb = new PathGradientBrush(pathGlow)) {
          pgb.CenterColor = Color.FromArgb(200,210,35,130);
          pgb.SurroundColors = new[] { Color.FromArgb(0,10,5,24) };
          g.FillPath(pgb, pathGlow);
        }
      }

      var gold = Color.FromArgb(255,255,215,70);
      var fillK = Color.FromArgb(255,245,45,125);
      var fillT = Color.FromArgb(255,130,55,245);
      var fillV = Color.FromArgb(255,35,210,250);
      float cx = size/2f, cy = size/2f;
      float cardW = size*0.30f, cardH = size*0.44f;
      float frontW = size*0.34f, frontH = size*0.50f;
      float fs = Math.Max(18f, size*0.16f);
      float ffs = Math.Max(22f, size*0.19f);

      DrawCard(g, cx - size*0.175f, cy + size*0.015f, cardW, cardH, -16f, fillK, gold, "K", fs);
      DrawCard(g, cx + size*0.175f, cy + size*0.015f, cardW, cardH,  16f, fillV, gold, "V", fs);
      DrawCard(g, cx, cy - size*0.01f, frontW, frontH, 0f, fillT, gold, "T", ffs);

      using (var penRing = new Pen(gold, Math.Max(4f, size*0.02f)))
        g.DrawEllipse(penRing, size*0.025f, size*0.025f, size*0.95f, size*0.95f);

      Directory.CreateDirectory(Path.GetDirectoryName(path));
      bmp.Save(path, ImageFormat.Png);
      Console.WriteLine("Wrote " + path + " (" + size + ")");
    }
  }

  static void WriteMenuBg(int w, int h, string path) {
    using (var bmp = new Bitmap(w, h))
    using (var g = Graphics.FromImage(bmp)) {
      g.SmoothingMode = SmoothingMode.AntiAlias;
      using (var lg = new LinearGradientBrush(new Rectangle(0,0,w,h),
        Color.FromArgb(255,6,3,16), Color.FromArgb(255,55,14,78), 270f))
        g.FillRectangle(lg, 0, 0, w, h);

      // secondary bloom
      using (var pathGlow = new GraphicsPath()) {
        pathGlow.AddEllipse(w*0.1f, h*0.05f, w*0.8f, h*0.45f);
        using (var pgb = new PathGradientBrush(pathGlow)) {
          pgb.CenterColor = Color.FromArgb(90,180,30,120);
          pgb.SurroundColors = new[] { Color.FromArgb(0,0,0,0) };
          g.FillPath(pgb, pathGlow);
        }
      }

      using (var penM = new Pen(Color.FromArgb(100,255,50,150), 5f))
      using (var penC = new Pen(Color.FromArgb(80,50,230,255), 3.5f)) {
        g.DrawLine(penM, -20, (int)(h*0.28), w+20, (int)(h*0.58));
        g.DrawLine(penC, -20, (int)(h*0.72), w+20, (int)(h*0.32));
      }

      var gold = Color.FromArgb(70,255,215,80);
      DrawCard(g, w*0.2f, h*0.38f, w*0.2f, h*0.17f, -18f, Color.FromArgb(55,255,40,120), gold, "", 1);
      DrawCard(g, w*0.8f, h*0.44f, w*0.18f, h*0.15f, 14f, Color.FromArgb(55,40,200,255), gold, "", 1);
      DrawCard(g, w*0.5f, h*0.24f, w*0.22f, h*0.18f, 3f, Color.FromArgb(60,120,40,200), gold, "", 1);

      var rnd = new Random(7);
      using (var brush = new SolidBrush(Color.FromArgb(220,255,225,130)))
      using (var brush2 = new SolidBrush(Color.FromArgb(160,120,240,255))) {
        for (int i=0;i<100;i++) {
          int sx = rnd.Next(0,w), sy = rnd.Next(0,h), ss = rnd.Next(1,3);
          g.FillEllipse(i%5==0 ? brush2 : brush, sx, sy, ss, ss);
        }
      }

      using (var pathV = new GraphicsPath()) {
        pathV.AddEllipse(-w*0.15f, -h*0.05f, w*1.3f, h*1.15f);
        using (var pgb = new PathGradientBrush(pathV)) {
          pgb.CenterColor = Color.FromArgb(0,0,0,0);
          pgb.SurroundColors = new[] { Color.FromArgb(170,0,0,0) };
          g.FillRectangle(pgb, 0, 0, w, h);
        }
      }

      Directory.CreateDirectory(Path.GetDirectoryName(path));
      bmp.Save(path, ImageFormat.Png);
      Console.WriteLine("Wrote " + path);
    }
  }

  static void Main() {
    string icons = @"C:\Users\Shikagera\proekti\unity\Kartovasya\Assets\Kartovasiya\Art\Icons";
    string ui = @"C:\Users\Shikagera\proekti\unity\Kartovasya\Assets\Kartovasiya\Art\UI";
    foreach (var s in new[]{1024,512,192,144,96,72,48})
      WriteIcon(s, Path.Combine(icons, s==512 ? "AppIcon.png" : ("AppIcon_"+s+".png")));
    // also keep AppIcon_512
    WriteIcon(512, Path.Combine(icons, "AppIcon_512.png"));
    WriteMenuBg(540, 960, Path.Combine(ui, "MenuBackdrop.png"));
    Console.WriteLine("DONE");
  }
}
