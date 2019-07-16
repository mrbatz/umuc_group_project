from flask import current_app, g
from app import db

class User(db.Model):
    user_id = db.Column(db.Integer, primary_key=True)
    user_name = db.Column(db.String(50), unique=True, nullable=False)
    user_pwd = db.Column(db.String(20), nullable=False)

    def __repr__(self):
        return '<User %r>' % self.username

class Agent(db.Model):
    agent_id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer, db.ForeignKey('user.user_id'), nullable=False)
    hostname = db.Column(db.String(200), nullable=False)
    ip_address = db.Column(db.String(200), nullable=False)
    softwares = db.relationship('AgentSoftware', backref='agent', lazy=True)
    traces = db.relationship('AgentTraces', backref='agent', lazy=True)

    def __repr__(self):
        return '<Agent %r>' % self.username

class Software(db.Model):
    software_id = db.Column(db.Integer, primary_key=True)
    software_name = db.Column(db.String(200), nullable=False)
    software_version = db.Column(db.String(50), nullable=False)
    release_date = db.Column(db.DateTime, nullable=False)

    def __repr__(self):
        return '<Software %r>' % self.username

class AgentSoftware(db.Model):
    agent_id = db.Column(db.Integer, db.ForeignKey('agent.agent_id'), nullable=False, primary_key=True)
    software_id = db.Column(db.Integer, db.ForeignKey('software.software_id'), nullable=False, primary_key=True)
    is_current = db.Column(db.Boolean)
    last_updated = db.Column(db.DateTime)

    def __repr__(self):
        return '<AgentSoftware %r>' % self.username

class AgentTraces(db.Model):
    agent_id = db.Column(db.Integer, db.ForeignKey('agent.agent_id'), nullable=False, primary_key=True)
    last_pinged = db.Column(db.DateTime, primary_key=True)

    def __repr__(self):
        return '<AgentTraces %r>' % self.username