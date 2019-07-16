DROP TABLE IF EXISTS agent_traces;
DROP TABLE IF EXISTS agent_softwares;
DROP TABLE IF EXISTS softwares;
DROP TABLE IF EXISTS agents;
DROP TABLE IF EXISTS users;

CREATE TABLE users
(
    user_id    NUMBER        PRIMARY KEY,
    user_name  VARCHAR2(50)  NOT NULL,
    user_pwd   VARCHAR2(20)  NOT NULL
);

CREATE TABLE agents
(
    agent_id    NUMBER         PRIMARY KEY,
    user_id     NUMBER         REFERENCES users(user_id) NOT NULL,
    hostname    VARCHAR2(200)  NOT NULL,
    ip_address  VARCHAR2(200)  NOT NULL
);

CREATE TABLE softwares
(
    software_id       NUMBER         PRIMARY KEY,
    software_name     VARCHAR2(200)  NOT NULL,
    software_version  VARCHAR2(50)   NOT NULL,
    release_date      DATE           NOT NULL,
    UNIQUE (software_name, software_version)
);

CREATE TABLE agent_softwares
(
    agent_id      NUMBER        REFERENCES agents(agent_id),
    software_id   NUMBER        REFERENCES softwares(software_id),
    is_current    VARCHAR2(1)   CHECK (is_current IN ('Y', 'N')),
    last_updated  DATE          DEFAULT (SYSDATE),
    PRIMARY KEY (agent_id, software_id)
);

CREATE TABLE agent_traces
(
    agent_id     NUMBER        REFERENCES agents(agent_id),
    last_pinged  DATE          DEFAULT (SYSDATE),
    PRIMARY KEY (agent_id, last_pinged)
);