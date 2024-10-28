#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#define _USE_MATH_DEFINES
#include <math.h>

// Global constants for user interaction
const float ZOOM_FACTOR = 1.1f;
const float CAMERA_SPEED = 0.02f;
const float MAX_ZOOM = 50.0f;
const float MIN_ZOOM = 10.0f;

// Global variables
float camera_angle = 0.0f;
float camera_height = 15.0f;
float zoom_level = 30.0f;
bool pause_simulation = false;
int focused_planet = -1;
bool show_help = true;
bool show_orbits = true;

class CelestialBody {
protected:
    float radius;
    float distance;
    float orbit_speed;
    float rotation_speed;
    float current_angle;
    float rotation_angle;
    float r, g, b;
    std::string name;
    bool has_atmosphere;

public:
    CelestialBody(float rad, float dist, float orb_speed, float rot_speed,
        float red, float green, float blue, std::string n, bool atm = false)
        : radius(rad), distance(dist), orbit_speed(orb_speed), rotation_speed(rot_speed),
        current_angle(0), rotation_angle(0), r(red), g(green), b(blue),
        name(n), has_atmosphere(atm) {}

    virtual void draw() = 0;
    virtual void drawOrbit() {
        if (!show_orbits) return;

        glPushMatrix();
        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 360; i++) {
            float angle = i * M_PI / 180.0f;
            float x = distance * cos(angle);
            float z = distance * sin(angle);
            glVertex3f(x, 0.0f, z);
        }
        glEnd();
        glPopMatrix();
    }

    void update() {
        if (!pause_simulation) {
            current_angle += orbit_speed;
            rotation_angle += rotation_speed;
            if (current_angle > 360.0f) current_angle -= 360.0f;
            if (rotation_angle > 360.0f) rotation_angle -= 360.0f;
        }
    }

    void position() {
        float x = distance * cos(current_angle * M_PI / 180.0f);
        float z = distance * sin(current_angle * M_PI / 180.0f);
        glTranslatef(x, 0.0f, z);
        glRotatef(rotation_angle, 0.0f, 1.0f, 0.0f);
    }

    std::string getName() const { return name; }
    float getDistance() const { return distance; }

    virtual ~CelestialBody() {}
};

class Star : public CelestialBody {
public:
    Star(float rad, float red, float green, float blue, std::string name)
        : CelestialBody(rad, 0, 0, 1, red, green, blue, name) {}

    void draw() override {
        // Draw glow effect
        glPushMatrix();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        float glow_scale = 1.1f;
        for (int i = 0; i < 5; i++) {
            glColor4f(r, g * 0.8f, 0, 0.1f);
            glutSolidSphere(radius * glow_scale, 30, 30);
            glow_scale += 0.1f;
        }

        // Draw core
        glColor3f(r, g, b);
        glutSolidSphere(radius, 30, 30);

        glDisable(GL_BLEND);
        glPopMatrix();

    }

    void drawOrbit() override {} // Stars don't have orbits
};

class Planet : public CelestialBody {
private:
    bool has_rings;
    float ring_color[3];
    float atmosphere_color[4];

public:
    Planet(float rad, float dist, float orb_speed, float rot_speed,
        float red, float green, float blue, std::string name,
        bool rings = false, bool atmosphere = false)
        : CelestialBody(rad, dist, orb_speed, rot_speed, red, green, blue, name, atmosphere),
        has_rings(rings) {
        // Initialize ring color
        ring_color[0] = red * 0.9f;
        ring_color[1] = green * 0.9f;
        ring_color[2] = blue * 0.9f;

        // Initialize atmosphere color
        atmosphere_color[0] = red * 0.7f;
        atmosphere_color[1] = green * 0.7f;
        atmosphere_color[2] = blue * 0.7f;
        atmosphere_color[3] = 0.3f;
    }

    void draw() override {
        glPushMatrix();

        // Draw atmosphere if planet has one
        if (has_atmosphere) {
            drawAtmosphere();
        }

        // Draw planet surface with simple shading
        glEnable(GL_LIGHTING);
        GLfloat mat_ambient[] = { r * 0.3f, g * 0.3f, b * 0.3f, 1.0f };
        GLfloat mat_diffuse[] = { r, g, b, 1.0f };
        GLfloat mat_specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
        GLfloat mat_shininess[] = { 50.0f };

        glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
        glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

        glutSolidSphere(radius, 30, 30);

        // Draw rings if planet has them
        if (has_rings) {
            drawRings();
        }

        glPopMatrix();

    }

private:
    void drawAtmosphere() {
        glPushMatrix();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4fv(atmosphere_color);
        glutSolidSphere(radius * 1.1f, 30, 30);

        glDisable(GL_BLEND);
        glPopMatrix();
    }

    void drawRings() {
        glPushMatrix();
        glRotatef(45.0f, 1.0f, 0.0f, 0.0f);

        // Draw multiple rings with varying colors
        for (float r = radius * 1.5f; r <= radius * 2.2f; r += radius * 0.1f) {
            glColor3f(ring_color[0], ring_color[1], ring_color[2]);
            glutSolidTorus(radius * 0.02f, r, 20, 40);
        }

        glPopMatrix();
    }
};

std::vector<CelestialBody*> solar_system;

void drawHelp() {
    if (!show_help) return;

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1.0f, 1.0f, 1.0f);
    std::vector<std::string> help_text = {
        "Controls:",
        "H - Toggle help",
        "P - Pause/Resume simulation",
        "O - Toggle orbit paths",
        "Mouse wheel - Zoom in/out",
        "Arrow keys - Adjust camera height",
        "1-9 - Focus on planets",
        "ESC - Exit"
    };

    float y = glutGet(GLUT_WINDOW_HEIGHT) - 20.0f;
    for (const auto& line : help_text) {
        glRasterPos2f(10.0f, y);
        for (char c : line) {
            glutBitmapCharacter(GLUT_BITMAP_9_BY_15, c);
        }
        y -= 20.0f;
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}

void init() {
    // Enable lighting and depth testing
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Set up light
    GLfloat light_position[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat light_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat light_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    // Create celestial bodies
    solar_system.push_back(new Star(2.0f, 1.0f, 0.7f, 0.0f, "Sun"));
    solar_system.push_back(new Planet(0.4f, 4.0f, 4.7f, 2.0f, 0.8f, 0.6f, 0.4f, "Mercury"));
    solar_system.push_back(new Planet(0.9f, 7.0f, 3.5f, 1.8f, 0.9f, 0.7f, 0.5f, "Venus", false, true));
    solar_system.push_back(new Planet(1.0f, 10.0f, 3.0f, 1.0f, 0.2f, 0.5f, 1.0f, "Earth", false, true));
    solar_system.push_back(new Planet(0.6f, 13.0f, 2.4f, 1.0f, 1.0f, 0.4f, 0.2f, "Mars"));
    solar_system.push_back(new Planet(1.8f, 17.0f, 1.3f, 0.8f, 0.9f, 0.7f, 0.5f, "Jupiter", false, true));
    solar_system.push_back(new Planet(1.6f, 21.0f, 0.9f, 0.7f, 0.9f, 0.8f, 0.6f, "Saturn", true));
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Position camera
    float cam_x = sin(camera_angle) * zoom_level;
    float cam_z = cos(camera_angle) * zoom_level;
    gluLookAt(cam_x, camera_height, cam_z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    // Draw celestial bodies
    for (auto body : solar_system) {
        glPushMatrix();
        body->drawOrbit();
        body->position();
        body->draw();
        glPopMatrix();
        body->update();
    }

    drawHelp();
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)w / h, 1.0f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

void update(int value) {
    if (!pause_simulation) {
        camera_angle += CAMERA_SPEED;
    }
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 27: // ESC
        for (auto body : solar_system) {
            delete body;
        }
        solar_system.clear();
        exit(0);
        break;
    case 'h':
    case 'H':
        show_help = !show_help;
        break;
    case 'p':
    case 'P':
        pause_simulation = !pause_simulation;
        break;
    case 'o':
    case 'O':
        show_orbits = !show_orbits;
        break;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        focused_planet = key - '1';
        if (focused_planet >= 0 && focused_planet < solar_system.size()) {
            zoom_level = solar_system[focused_planet]->getDistance() * 1.5f;
            zoom_level = std::max(MIN_ZOOM, std::min(zoom_level, MAX_ZOOM));
        }
        break;
    }
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_UP:
        camera_height += 0.5f;
        break;
    case GLUT_KEY_DOWN:
        camera_height -= 0.5f;
        break;
    }
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    if (state == GLUT_UP) return;

    switch (button) {
    case 3: // Scroll up
        zoom_level /= ZOOM_FACTOR;
        if (zoom_level < MIN_ZOOM) zoom_level = MIN_ZOOM;
        break;
    case 4: // Scroll down
        zoom_level *= ZOOM_FACTOR;
        if (zoom_level > MAX_ZOOM) zoom_level = MAX_ZOOM;
        break;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Solar System Simulation");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);
    glutTimerFunc(0, update, 0);

    glutMainLoop();
    return 0;
}