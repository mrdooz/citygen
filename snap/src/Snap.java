import processing.core.PApplet;
import processing.core.PFont;
import processing.core.PVector;

public class Snap extends PApplet {

    PFont f = createFont("Arial",16,true); // Arial, 16 point, anti-aliasing on

    PVector[] points = new PVector[4];
    int curPoint = 0;

    public void setup() {
        size(400, 400);
        stroke(255);

        for (int i = 0; i < 4; ++i)
            points[i] = new PVector();
    }

    public void drawLine(PVector a, PVector b)
    {
        line(a.x, a.y, b.x, b.y);
    }

    public void drawLineTest() {
        switch (curPoint) {
            case 0:
                break;
            case 1:
                line(points[0].x, points[0].y, mouseX, mouseY);
                break;
            case 2:
                drawLine(points[0], points[1]);
                break;
            case 3:
                drawLine(points[0], points[1]);
                line(points[2].x, points[2].y, mouseX, mouseY);
                break;
            case 4:
                drawLine(points[0], points[1]);
                drawLine(points[2], points[3]);
                break;
        }

        if (curPoint == 4) {
            float x1 = points[0].x; float x2 = points[1].x; float x3 = points[2].x; float x4 = points[3].x;
            float y1 = points[0].y; float y2 = points[1].y; float y3 = points[2].y; float y4 = points[3].y;

            float numx  = (x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3);
            float numy  = (x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3);
            float denom = (y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1);

            float ua = numx / denom;
            float ub = numy / denom;

            float x = x1 + ua * (x2 - x1);
            float y = y1 + ua * (y2 - y1);

            // the line segments intersect if both ua and ub are in the [0..1] range

            text(String.format("x: %.2f, y: %.2f, ua: %.2f, ub: %.2f", x, y, ua, ub), 0, 20);
        }
    }

    public void mouseClicked() {
        if (curPoint == 4)
            curPoint = 0;

        points[curPoint].x = mouseX;
        points[curPoint].y = mouseY;

        curPoint++;
    }

    public void linePointTest() {
        float w = width / 2;

        PVector a = new PVector(w, height * 0.75f);
        PVector b = new PVector(w, height * 0.25f);

        PVector p = new PVector(mouseX, mouseY);

        PVector ab = PVector.sub(b, a);
        PVector ap = PVector.sub(p, a);
        PVector bp = PVector.sub(p, b);

        drawLine(a, b);
        drawLine(a, p);
        drawLine(b, p);

        // check if the intersection point is within the segment
        if (PVector.dot(ab, ap) < 0)
            return;

        if (PVector.dot(ab, bp) > 0)
            // the intersection pt is above the end node, so we want to
            // do a pt/pt distance check here
            return;

        PVector n = new PVector();
        n.set(ab);
        n.normalize();

        // calc projection of ap onto ab
        PVector proj = PVector.mult(n, ap.dot(n));

        // calc vector between projection and p
        PVector dist = PVector.sub(ap, proj);
        float d = dist.mag();

        text(String.format("n: (%.2f, %.2f), d: %.2f, dot(ab,pa): %.2f, dot(ab, pb): %.2f",
                n.x, n.y,
                d, PVector.dot(ab, ap), PVector.dot(ab, bp)), 0, 20);

    }

    public void draw() {

        background(192, 64, 0);
        textFont(f, 16);
        fill(255);
//        linePointTest();
        drawLineTest();

    }
}