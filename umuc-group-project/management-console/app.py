from flask import Flask, render_template, redirect, url_for, request, session, g, flash
from flask_sqlalchemy import SQLAlchemy
from passlib.hash import sha256_crypt
import os
import hashlib
import datetime
from waitress import serve
from json import dumps

app = Flask(__name__)

app.secret_key = os.urandom(24)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:////tmp/app.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
db = SQLAlchemy(app)
now = datetime.datetime.now()


@app.route('/')
def index():
    if 'username' in session:
        username = session['username']
        return redirect(url_for('dashboard'))

    return redirect(url_for('login'))


@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']

        # find user from database
        user = User.query.filter_by(name=username).first()

        if(user and sha256_crypt.verify(password, user.password)):
            session['logged_in'] = True
            session['username'] = user.name
            return redirect(url_for('index'))
        else:
            flash('Invalid login attempt. Please try again.', 'danger')
            return render_template('login.html')

    return render_template('login.html')

@app.route('/logout', methods=['GET'])
def logout():
    session.pop('username', None)
    return redirect(url_for('index')) 

@app.route('/register', methods=['GET', 'POST'])
def register():
    if 'username' in session:
        return redirect(url_for('index'))
    
    if request.method == 'POST':
        username = request.form['username']
        password = sha256_crypt.encrypt(request.form['password'])

        user = User.query.filter_by(name=username).first()
        
        if(user):
            flash('User account already exist. Please try again.', 'danger')
            return render_template('registration.html')
        else:
            try:
                new_user = User(username, password)
                db.session.add(new_user)
                db.session.commit()

                session['logged_in'] = True
                session['username'] = new_user.name
                return redirect(url_for('index'))
            except Exception as e:
                print(e)
                flash('Unable to register new account.', 'danger')
                return render_template('registration.html')

    return render_template('registration.html')

@app.route('/dashboard')
def dashboard():
    if 'username' in session:
        return render_template('dashboard.html')
    else:
        return redirect(url_for('index'))


@app.route('/hosts')
def hosts():
    if 'username' in session:
        agents = Agent.query.all()
        return render_template('hosts.html', agents=agents)

    else:
        return redirect(url_for('index'))


@app.route('/tasks')
def tasks():
    if 'username' in session:

        return render_template('tasks.html')

    else:
        return redirect(url_for('index'))


@app.route('/settings')
def settings():
    if 'username' in session:

        return render_template('settings.html')

    else:
        return redirect(url_for('index'))


@app.route('/beacon', methods=['GET', 'POST'])
def beacon():
    agent_data = None
    if request.method == 'POST':
        agent_data = request.get_json()
        f = open("/tmp/management_console.log", "a");
        f.write(dumps(agent_data) + "\n");
        f.close();
    if request.method == 'GET':  # temporary until we can get json data from agent
        agent_data = request.args

    # TODO: Process, decode and store agent_data
    if (agent_data):
        print(agent_data)
    else:
        print("No agent data.")
    return 'Succeess or failure'

def process_agent_data(agent_data):
    # create agent
    agent = Agent.query.filter_by(hostanme=agent_hostname).first()
    agent_hostname = agent_data.get("Hostname")
    ip_address = agent_data.get('ip_address')
    if( agent == None):
        agent = Agent(hostname, ip_address)
        db.session.add(agent)
        db.session.commit()

    # create sofware
    software_name = agent_data.get("Package Name")
    software_version = agent_data.get("Version")
    software = Software(software_name, software_version)

    # create agent sofware relationship
    agent_software = AgentSoftware(agent.agent_id, software.software_id)


class User(db.Model):
    user_id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(50), unique=True, nullable=False)
    password = db.Column(db.String(20), nullable=False)

    def __init__(self, name=None, password=None):
        self.name = name
        self.password = password


class Agent(db.Model):
    agent_id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer, db.ForeignKey(
        'user.user_id'), nullable=False)
    hostname = db.Column(db.String(200), nullable=False)
    ip_address = db.Column(db.String(200), nullable=False)
    softwares = db.relationship('AgentSoftware', backref='agent', lazy=True)
    traces = db.relationship('AgentTraces', backref='agent', lazy=True)

    def __init__(self, hostname=None, ip_address=None):
        self.hostname = hostname
        self.ip_address = ip_address


class Software(db.Model):
    software_id = db.Column(db.Integer, primary_key=True)
    software_name = db.Column(db.String(200), nullable=False)
    software_version = db.Column(db.String(50), nullable=False)
    release_date = db.Column(db.DateTime)

    def __init__(self, software_name=None, software_version=None):
        self.software_name = software_name
        self.software_version = software_version


class AgentSoftware(db.Model):
    agent_id = db.Column(db.Integer, db.ForeignKey(
        'agent.agent_id'), nullable=False, primary_key=True)
    software_id = db.Column(db.Integer, db.ForeignKey(
        'software.software_id'), nullable=False, primary_key=True)
    is_current = db.Column(db.Boolean)
    last_updated = db.Column(db.DateTime)

    def __init__(self, agent_id=None, software_id=None, is_current=None, last_updated=None):
        self.agent_id = agent_id
        self.software_id = software_id


class AgentTraces(db.Model):
    agent_id = db.Column(db.Integer, db.ForeignKey(
        'agent.agent_id'), nullable=False, primary_key=True)
    last_pinged = db.Column(db.DateTime, primary_key=True)

    def __init__(self, agent_id=None, last_pinged=None):
        self.agent_id = agent_id
        self.last_pinged = last_pinged


# Build the database:
# This will create the database file using SQLAlchemy
db.create_all()

if __name__ == "__main__":
    # get admin user
    admin = User.query.filter_by(name='admin').first()
    # check if admin user exists
    if(admin == None):
        # create admin user since it does not exist
        name = 'admin'
        password = sha256_crypt.encrypt('admin')
        admin_user = User(name, password)
        db.session.add(admin_user)
        db.session.commit()
#    app.run(host="0.0.0.0", debug=True, threaded=True)
    serve(app, host='0.0.0.0', port=5000)
