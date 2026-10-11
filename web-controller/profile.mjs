// Calendar-day arithmetic deliberately ignores the host timezone and DST.
export function birthdayCountdown(birthday, localDate) {
  if (!/^\d{2}-\d{2}$/.test(birthday || '')) return null;
  const [month,day]=birthday.split('-').map(Number);
  if(month<1||month>12||day<1||day>[31,29,31,30,31,30,31,31,30,31,30,31][month-1])return null;
  const match=/^(\d{4})-(\d{2})-(\d{2})(?:T|$)/.exec(localDate || '');
  if(!match)return null;
  const [year,m,d]=match.slice(1).map(Number),today=Date.UTC(year,m-1,d),check=new Date(today);
  if(check.getUTCFullYear()!==year||check.getUTCMonth()!==m-1||check.getUTCDate()!==d)return null;
  let target=Date.UTC(year,month-1,day);
  if(target<today)target=Date.UTC(year+1,month-1,day);
  return {days:Math.round((target-today)/86400000),date:new Date(target).toISOString().slice(0,10)};
}
