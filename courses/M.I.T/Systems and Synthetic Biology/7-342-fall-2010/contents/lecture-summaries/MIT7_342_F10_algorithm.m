% Modified Gillespie Algorithm:
% Sept. 21, 10 [Tues.]

clear;
close all;

totTrials = 5000;
A =100; % Mean production rate.
gamma = 1; % Rate of degradation per particle.

% Reactions:
% 1. Proliferation of particle.
% 2. Particle degradation.

n=3; % Starting number of particles.
theta = zeros(1,2);
r = zeros(1,2);
trajectory = zeros(1,totTrials+1);
time = zeros(1,totTrials+1);

for i=1:totTrials
    % random(uniform distn,0,1, row, no. of columns)
    trajectory(i)=n;
    r = [A gamma*n];
    u = random('unif',0,1,1,2); 
    theta = -log(u)./r;
    if (n==0)
        n = n+1;
        time(i+1) = time(i)+theta(1);
    elseif (theta(1) < theta(2))
        n = n+1;
        time(i+1) = time(i)+theta(1);
    else
        n = n-1;
        time(i+1)=time(i)+theta(2);
    end
end

trajectory(totTrials+1)=n;

hold off
plot(time,trajectory,'--r');
hold on

time2 = 0:time(totTrials+1)/(totTrials):time(totTrials);
meanParticle = (A/gamma)*(1-exp(-gamma*time2));
plot(time2,meanParticle,'-k');
axis tight
xlabel('Time (a.u.)');
ylabel('Number of particles');
time(totTrials+1)








    